#pragma once
#include <iostream>
#include <exception>
#include <thread>
#include <vector>
#include "nn.hpp"

namespace utils {

    namespace rpc {
        // /req/[topic]/[uuid]:[data] 提问消息格式
        // /rep/[topic]/[uuid]:[data] 应答消息格式
        class client            // client
        {
        public:
            client () : pub_sock_(AF_SP, NN_PUB)
            {
                pub_sock_.connect(IPC_SUBSCRIBE_SOCKET_PATH);
            }
            virtual ~client () {
            }
            // 一问一答，同步请求
            std::string request (std::string &&method, std::string &&data, int timeout = 3000)
            {
                std::string subject = "/rep/" + method + "/";
                nn::socket sub_sock (AF_SP, NN_SUB);
                sub_sock.setsockopt (NN_SUB, NN_SUB_SUBSCRIBE, subject.c_str(), subject.length());
                sub_sock.connect (IPC_BROADCAST_SOCKET_PATH);
                sub_sock.setsockopt (NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof (timeout));

                // 发送远程调用
                subject.replace(0, 5, "/req/");
                std::string sndbuf = subject + ":" + data;
                std::cout << "发送消息 : " << sndbuf << "\n";
                pub_sock_.send (sndbuf.c_str(), sndbuf.size() + 1, 0);

                // 收取回复结果
                try {
                    char * buf = nullptr;
                    int size = sub_sock.recv(&buf, NN_MSG, 0);
                    std::string ret = strchr(buf, ':') + 1;
                    nn::freemsg(buf);
                    return ret;
                } catch (std::exception &e) {
                    // 判断是否是超时引起
                    return "";
                }
            }
            // 一问多答，收集结果
            std::vector<std::string> survey (std::string &&method, std::string &&data, int timeout = 500)
            {
                std::string subject = "/rep/" + method + "/";
                nn::socket sub_sock (AF_SP, NN_SUB);
                sub_sock.setsockopt (NN_SUB, NN_SUB_SUBSCRIBE, subject.c_str(), subject.length());
                sub_sock.connect (IPC_BROADCAST_SOCKET_PATH);
                sub_sock.setsockopt (NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof (timeout));

                // 发送远程调用
                subject.replace(0, 5, "/req/");
                std::string sndbuf = subject + ":" + data;
                std::cout << "发送消息 : " << sndbuf << "\n";
                pub_sock_.send (sndbuf.c_str(), sndbuf.size() + 1, 0);

                std::vector<std::string> ret;
                // 收取回复结果
                try {
                    while (1) {
                        char * buf = nullptr;
                        int size = sub_sock.recv(&buf, NN_MSG, 0);
                        ret.push_back(strchr(buf, ':') + 1);
                        nn::freemsg(buf);
                    }
                } catch (std::exception &e) {
                    // 判断是否是超时引起
                }
                return ret;
            }
            // 异步请求，返回一个future 信息，进程间速度很快，暂时不做
            // 单发
            void singleshot (std::string &&method, std::string &&data)
            {
                // 发送远程调用
                std::string sndbuf = "/req/" + method + ":" + data;
                std::cout << "发送消息 : " << sndbuf << "\n";
                pub_sock_.send (sndbuf.c_str(), sndbuf.size() + 1, 0);
            }
        
        private:
            nn::socket pub_sock_;
        };

    }  // rpc

}  // utils
