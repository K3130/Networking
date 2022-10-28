#pragma once

#include "net_common.h"

namespace net
{
	/// SESSION

	class Session : public std::enable_shared_from_this<Session>
	{
	public:
		Session(io::io_context& aContext, tcp::socket aSocket) : m_io_context(aContext), m_socket(std::move(aSocket)) {}

		virtual ~Session() {}

		void start(message_handler&& on_message, error_handler&& on_error) 
		{
			this->m_on_message = std::move(on_message);
			this->m_on_error = std::move(on_error);
			async_read();
		}

		void send_message_in_queue(const std::string& aMessage)
		{
			bool idle = m_outgoing.empty();
			m_outgoing.push(aMessage);

			if (idle) {
				async_write();
			}
		}
	private:
		void async_read()
		{
			asio::async_read_until(m_socket, m_streambuf, "\n", std::bind(&Session::on_read, shared_from_this(), _1, _2));
		}

		void on_read(error_code aError, std::size_t aBytesTransferred)
		{
			if (!aError)
			{
				//read message in stream
				std::stringstream message;
				message << std::istream(&m_streambuf).rdbuf();
				
				m_streambuf.consume(aBytesTransferred);
				m_on_message(message.str());
				async_read();
			}
			else
			{
				m_socket.close(aError);
			}
		}

		void async_write()
		{
			asio::async_write(m_socket, io::buffer(m_outgoing.front()), std::bind(&Session::on_write, shared_from_this(), _1, _2));
		}

		void on_write(error_code aError, std::size_t aBytesTransferred) {
			if (!aError) {
				m_outgoing.pop();

				if (!m_outgoing.empty()) {
					async_write();
				}
			}
			else {
				m_socket.close(aError);
			}
		}
	public:
		bool is_connected() const
		{
			return m_socket.is_open();
		}

		void set_id(uint32_t aId)
		{
			m_id = aId;
		}

		uint32_t get_id() const
		{
			return m_id;
		}

		void disconnect()
		{
			if (is_connected())
				asio::post(m_io_context, [this]() { m_socket.close(); });
		}

		std::queue<std::string> get_message_from_client() const
		{
			return m_outgoing;
		}


	protected:
		io::io_context& m_io_context;
		tcp::socket m_socket;
		io::streambuf m_streambuf;
		std::queue<std::string> m_outgoing;
		message_handler m_on_message;
		error_handler m_on_error;
		uint32_t m_id = 0;
	};


	/// SERVER


	class Server
	{
	public:
		Server(uint16_t aPort) : m_acceptor(m_io_context, tcp::endpoint(tcp::v4(), aPort))
		{

		}

		virtual ~Server()
		{
			stop_server();
		}


		void wait_for_client_connection()
		{
			m_acceptor.async_accept([this](error_code error, tcp::socket socket)
				{
					if (!error)
					{

						std::shared_ptr<Session> new_client = std::make_shared<Session>(m_io_context, std::move(socket));


						if (on_client_connect(new_client))
						{
							m_clients.push_back(std::move(new_client));
							m_clients.back()->set_id(++m_id_counter);
							m_clients.back()->send_message_in_queue("[SERVER] Welcome to server, you id " + std::to_string(m_clients.back()->get_id()) + "\n\r");
							
							
							//This sends a message to all clients that came to the server
							m_clients.back()->start(std::bind(&Server::send_message_all_clients, this, _1), nullptr);
						
						}

					}
					else
					{
						set_error(error.message());
					}

					wait_for_client_connection();
				});
			
		}

		void send_message_all_clients(const std::string& aMessage)
		{
			bool invalid_clients = false;

			for (auto& client : m_clients)
			{
				if (client && client->is_connected())
				{				
					client->send_message_in_queue(aMessage);
					//handle
					on_client_message(client, aMessage);
				}
				else
				{
					on_client_disconnect(client);
					client.reset();
				}
			}

			if (invalid_clients)
				m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), nullptr), m_clients.end());
		}

		void send_message_for_client(const std::string& aMessage, std::shared_ptr<Session> aClient)
		{
			if (aClient && aClient->is_connected())
			{
				aClient->send_message_in_queue(aMessage);
			}
			else
			{
				on_client_disconnect(aClient);
				aClient.reset();
				m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), aClient), m_clients.end());
			}
		}

		void stop_server()
		{
			m_io_context.stop();

			if (m_thread_context.joinable())
				m_thread_context.join();
		}

		bool start_server()
		{
			try
			{
				wait_for_client_connection();
				m_thread_context = std::thread([this]() { m_io_context.run(); });
			}
			catch (const error_code& e)
			{
				set_error(e.message());
				return false;
			}
			return true;
		}

	public:
		const std::string get_error() const
		{
			return m_error_bufer;
		}

		std::deque < std::shared_ptr<Session>> getClients() const
		{
			return m_clients;
		}


	protected:
		virtual bool on_client_connect(std::shared_ptr<Session> aClient) { return true; }
		virtual void on_client_disconnect(std::shared_ptr<Session> aClient) {}
		virtual void on_client_message(std::shared_ptr<Session> aClient, const std::string& aMessage) {}

	protected:
		std::string m_error_bufer;
		void set_error(const std::string& aError) { m_error_bufer = aError; }
		std::deque<std::shared_ptr<Session>> m_clients;
		io::io_context m_io_context;
		std::thread m_thread_context;
		tcp::acceptor m_acceptor;
		uint32_t m_id_counter = 1000;
	};


}