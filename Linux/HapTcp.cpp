/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "Hap/Hap.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace Hap
{
	class TcpImpl : public Tcp
	{
	private:
		std::thread task;
		bool running = false;

		int server;
		int client[Hap::MaxHttpSessions + 1];
		Hap::sid_t sess[Hap::MaxHttpSessions + 1];

		static constexpr unsigned pollPeriod = 100000;		// 100000us = .1s 
		static constexpr unsigned pollLimit = 30 * 60 * 10;	// session timeout in poll periods
		unsigned pollCount[Hap::MaxHttpSessions];

		void run()
		{
			Log::Msg("Tcp::Run - enter\n");

			struct sockaddr_in address;
			size_t addrlen = sizeof(address);

			while (running)
			{
				int nfds = 0;
				fd_set readfds;
				fd_set exceptfds;

				FD_ZERO(&readfds);
				FD_ZERO(&exceptfds);

				FD_SET(server, &readfds);
				nfds = server + 1;

				for (unsigned i = 0; i < sizeofarr(client); i++)
				{
					int sd = client[i];
					if (sd > 0)
					{
						FD_SET(sd, &readfds);
						FD_SET(sd, &exceptfds);
						if (sd >= nfds)
							nfds = sd + 1;
					}
				}

				Log::Dbg("Tcp::Run - select\n");
				timeval to = { 0, 100000 };
				int rc = ::select(nfds, &readfds, NULL, &exceptfds, &to);
				Log::Dbg("Tcp::Run - select: %d\n", rc);
				if (rc < 0)
				{
					Log::Msg("select error %s\n", strerror(errno));
				}

				if (rc == 0)
				{
					// timeout, process events
					for (unsigned i = 0; i < sizeofarr(client); i++)
					{
						int sd = client[i];
						if (sd == 0)
							continue;

						Hap::sid_t sid = sess[i];
						if (sid == Hap::sid_invalid)
							continue;

						Log::Dbg("Tcp::Run - poll sid %d\n", sid);

						pollCount[i]++;

						_http->Poll(sid, [this, sd, i](Hap::sid_t sid, char* buf, uint16_t len) -> int
						{
							if (buf != nullptr)
							{
								pollCount[i] = 0;
								return ::send(sd, buf, len, 0);
							}
							return 0;
						});
					}
				}

				// read event on server socket - incoming connection
				if (FD_ISSET(server, &readfds))
				{
					Log::Dbg("Tcp::Run - accept %d\n", server);

					int clnt = ::accept(server, (struct sockaddr *)&address, &addrlen);
					if (clnt < 0)
					{
						Log::Msg("accept error\n");
					}
					else
					{
						Log::Msg("Connection on socket %d from ip %s  port %d\n", clnt,
							::inet_ntoa(address.sin_addr), ntohs(address.sin_port));

						//add new socket to array of sockets
						for (unsigned i = 0; i < sizeofarr(client); i++)
						{
							if (client[i] == 0)
							{
								client[i] = clnt;
								pollCount[i] = 0;
								break;
							}
						}
					}
				}

				// read event on client socket - data or disconnect
				for (unsigned i = 0; i < sizeofarr(client); i++)
				{
					int sd = client[i];
					Hap::sid_t sid = sess[i];
					bool close = false;

					if (FD_ISSET(sd, &readfds))
					{
						Log::Dbg("Tcp::Run - data from %d\n", sd);

						if (sid == Hap::sid_invalid)
						{
							sid = _http->Open();
							sess[i] = sid;
						}

						if (sid == Hap::sid_invalid)
						{
							Log::Msg("Cannot open HTTP session for client %d\n", i);
							close = true;
						}
						else
						{
							pollCount[i] = 0;

							bool rc = _http->Process(sid,
								[sd](Hap::sid_t sid, char* buf, uint16_t size) -> int
								{
									return ::recv(sd, buf, size, 0);
								},
								[sd](Hap::sid_t sid, char* buf, uint16_t len) -> int
								{
									if (buf != nullptr)
										return ::send(sd, buf, len, 0);
									return 0;
								}
							);

							if (!rc)
							{
								Log::Msg("Socket %d Disconnect\n", sd);
								close = true;
							}
						}
					}
					
					if (FD_ISSET(sd, &exceptfds))
					{
						Log::Msg("Socket %d error\n", sd);
						close = true;
					}

					if (pollCount[i] > pollLimit)
					{
						Log::Msg("Socket %d timeout\n", sd);
						pollCount[i] = 0;
						close = true;
					}

					if (close)
					{
						::getpeername(sd, (struct sockaddr*)&address, &addrlen);
						Log::Msg("Disconnect socket %d to ip %s  port %d\n", sd,
							::inet_ntoa(address.sin_addr), ntohs(address.sin_port));

						::close(sd);
						client[i] = 0;
						_http->Close(sid);
						sess[i] = sid_invalid;
					}
				}
			}

			for (unsigned i = 0; i < sizeofarr(client); i++)
			{
				int sd = client[i];
				if (sd != 0)
				{
					::close(sd);
					client[i] = 0;
				}
			}

			Log::Msg("Tcp::Run - exit\n");
		}

	public:
		TcpImpl()
		{
			server = 0;
			for (unsigned i = 0; i < sizeofarr(client); i++)
				client[i] = 0;
		}

		~TcpImpl()
		{
			Stop();
		}

		virtual bool Start() override
		{
			for (unsigned i = 0; i < sizeofarr(client); i++)
			{
				client[i] = 0;
				sess[i] = sid_invalid;
			}

			//create the server socket
			server = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (server < 0)
			{
				Log::Msg("server socket creation failed\n");
				return false;
			}

			// allow local address reuse
			int opt = 1;
			if (::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
			{
				Log::Msg("setsockopt(server, SO_REUSEADDR) failed: %s\n", strerror(errno));
				return false;
			}

			//bind the socket
			struct sockaddr_in address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = Hap::config->port;

			Log::Dbg("Tcp::Start - bind %d to %s:%d\n", server,
					::inet_ntoa(address.sin_addr), ntohs(address.sin_port));

			if (::bind(server, (struct sockaddr *)&address, sizeof(address))<0)
			{
				Log::Msg("bind(server, INADDR_ANY) failed\n");
				return false;
			}

			if (::listen(server, MaxHttpSessions) < 0)
			{
				Log::Msg("listen(server) failed\n");
				return false;
			}

			running = true;
			task = std::thread(&TcpImpl::run, this);

			return running;
		}

		virtual void Stop() override
		{
			running = false;

			::close(server);

			if (task.joinable())
				task.join();
		}

	} tcp;

	Tcp* Tcp::Create(Hap::Http::Server* http)
	{
		tcp._http = http;
		return &tcp;
	}
}

