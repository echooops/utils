#pragma once
#include <iostream>
#include <exception>
#include <thread>

#include "nn.hpp"

namespace utils {
    
    namespace rpc {

        class bus
        {
        public:
            bus () : running (true)
                   , broadcast_sock_ (AF_SP_RAW, NN_PUB)
                   , subscribe_sock_ (AF_SP_RAW, NN_SUB)
                   , broadcaster_thread_ (&bus::broadcaster, this)
            { }

            virtual ~bus() noexcept
            {
                running = false;
                // 关闭通道
                broadcaster_thread_.join();
            }
        
        private:
            // 广播员，收到消息就广播
            void broadcaster ()
            {
                try {
                    // 转发所有消息
                    subscribe_sock_.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
                
                    broadcast_sock_.bind (IPC_BROADCAST_SOCKET_PATH);
                    subscribe_sock_.bind (IPC_SUBSCRIBE_SOCKET_PATH);
                    // 阻塞，转发广播
                    nn::device(subscribe_sock_, broadcast_sock_);
                } catch (std::exception &e) {
                    std::cerr << "[nanomsg]" << e.what() << "\n";
                }
            }

        private:
            bool running;
            nn::socket broadcast_sock_;
            nn::socket subscribe_sock_;
            std::thread broadcaster_thread_;
        };

    }  // rpc
} // utils
