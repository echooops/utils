#pragma once
#include <cstdio>
#include <cstring>
#include <iostream>
#include <exception>
#include <functional>
#include <thread>
#include <map>

#include <thread/threadpool.hpp>
#include "nn.hpp"

namespace utils {

    namespace rpc {

        class server             // 处理订阅的任务
        {
        public:
            server () : running_ (false)
                      , sub_sock_ (AF_SP, NN_SUB)
                      , pub_sock_ (AF_SP, NN_PUB)
                      , pool (4) {
                pub_sock_.connect (IPC_SUBSCRIBE_SOCKET_PATH);
            }
            virtual ~server() { }
            // 开始接受订阅消息
            void start () {
                try {
                    running_ = true;
                    // 遍历订阅所有任务
                    for (auto e : methods_) {
                        // 只接收请求消息
                        std::string subject = "/req/" + e.first + "/";
                        sub_sock_.setsockopt (NN_SUB, NN_SUB_SUBSCRIBE, subject.c_str(), subject.length());
                        std::cout << subject << "\n";
                    }
                    // 连接广播
                    sub_sock_.connect (IPC_BROADCAST_SOCKET_PATH);
                    // 启动消息接收器
                    receiver_thread_ = std::thread (&server::receiver, this);

                } catch (std::exception &e) {
                    std::cerr << "[nanomsg]" << e.what() << "\n";
                }
            }

            void stop () {
                running_ = false;
                // 关闭sock
                receiver_thread_.join();
            }

            void receiver () {
                while (running_) {
                    char * buf = nullptr;
                    std::cout << "开始接收消息" << "\n";
                    size_t size = sub_sock_.recv(&buf, NN_MSG, 0);
                    // 拆解
                    std::cout << "收到消息 : size(" << size << ") len("
                              << strlen(buf) << ") data(" << buf << ")\n";
                    // 执行任务
                    pool.commit(std::bind(&server::worker, this, buf));

                    nn::freemsg(buf);
                }
            }
            // DONE: 需要一个线程池 线程池已经加入
            void worker (std::string str) {
                std::size_t pos = str.find(":");
                std::string head = str.substr(0, pos);
                std::string method = head.substr(5, head.rfind("/") - 5);
                std::string data = str.substr(pos + 1);

                std::cout << "head : " << head << "\n";
                std::cout << "method : " << method << "\n";
                std::cout << "data : " << data << "\n";
                // 处理信息
                std::string ret = methods_[method](data);
                // 回复结果
                head.replace(0, 5, "/rep/");
                std::string sndbuf = head + ":" + ret;
                std::cout << "回复消息:" << sndbuf << "\n";

                pub_sock_.send(sndbuf.c_str(), sndbuf.size() + 1, 0);
            }


            void register_methods (std::string topic, std::function<std::string(const std::string &)> func) {
                if (!running_)
                    methods_[topic] = std::move(func);
                else
                    std::cout << "topic 已订阅，若需插入新的话题，需要先stop，注册方法，再start" << "\n";
            }

        private:

            bool running_;

            nn::socket sub_sock_;
            nn::socket pub_sock_;

            std::thread receiver_thread_;
            std::map<std::string, std::function<std::string(std::string)>> methods_;

            utils::thread::threadpool pool; // 定义线程池
        };

    }  // rpc

}  // utils
