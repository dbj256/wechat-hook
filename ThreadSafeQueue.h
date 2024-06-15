
#ifndef COMMON_SOURCE_CPP_THREADSAFE_QUEUE_H_
#define COMMON_SOURCE_CPP_THREADSAFE_QUEUE_H_
#include <queue>
#include <mutex>
#include <condition_variable>
#include <initializer_list>
namespace gdface {
    inline namespace mt {
        /*
         * �̰߳�ȫ����
         * TΪ����Ԫ������
         * ��Ϊ��std::mutex��std::condition_variable���Ա,���Դ��಻֧�ָ��ƹ��캯��Ҳ��֧�ָ�ֵ������(=)
         * */
        template<typename T>
        class threadsafe_queue {
        private:
            // data_queue�����ź���
            mutable std::mutex mut;
            mutable std::condition_variable data_cond;
            using queue_type = std::queue<T>;
            queue_type data_queue;
        public:
            using value_type = typename queue_type::value_type;
            using container_type = typename queue_type::container_type;
            threadsafe_queue() = default;
            threadsafe_queue(const threadsafe_queue&) = delete;
            threadsafe_queue& operator=(const threadsafe_queue&) = delete;
            /*
             * ʹ�õ�����Ϊ�����Ĺ��캯��,����������������
             * */
            template<typename _InputIterator>
            threadsafe_queue(_InputIterator first, _InputIterator last) {
                for (auto itor = first; itor != last; ++itor) {
                    data_queue.push(*itor);
                }
            }
            explicit threadsafe_queue(const container_type& c) :data_queue(c) {}
            /*
             * ʹ�ó�ʼ���б�Ϊ�����Ĺ��캯��
             * */
            threadsafe_queue(std::initializer_list<value_type> list) :threadsafe_queue(list.begin(), list.end()) {
            }
            /*
             * ��Ԫ�ؼ������
             * */
            void push(const value_type& new_value) {
                std::lock_guard<std::mutex>lk(mut);
                data_queue.push(std::move(new_value));
                data_cond.notify_one();
            }
            /*
             * �Ӷ����е���һ��Ԫ��,�������Ϊ�վ�����
             * */
            value_type wait_and_pop() {
                std::unique_lock<std::mutex>lk(mut);
                data_cond.wait(lk, [this] {return !this->data_queue.empty(); });
                auto value = std::move(data_queue.front());
                data_queue.pop();
                return value;
            }
            /*
             * �Ӷ����е���һ��Ԫ��,�������Ϊ�շ���false
             * */
            bool try_pop(value_type& value) {
                std::lock_guard<std::mutex>lk(mut);
                if (data_queue.empty())
                    return false;
                value = std::move(data_queue.front());
                data_queue.pop();
                return true;
            }
            /*
             * ���ض����Ƿ�Ϊ��
             * */
            auto empty() const->decltype(data_queue.empty()) {
                std::lock_guard<std::mutex>lk(mut);
                return data_queue.empty();
            }
            /*
             * ���ض�����Ԫ������
             * */
            auto size() const->decltype(data_queue.size()) {
                std::lock_guard<std::mutex>lk(mut);
                return data_queue.size();
            }
        }; /* threadsafe_queue */
    }/* namespace mt */
}/* namespace gdface */
#endif /* COMMON_SOURCE_CPP_THREADSAFE_QUEUE_H_ */