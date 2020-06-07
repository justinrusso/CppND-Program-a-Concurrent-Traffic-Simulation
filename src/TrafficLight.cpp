#include <iostream>
#include <random>
#include "TrafficLight.h"

#include <future>

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    std::unique_lock<std::mutex> lock(_mutex);

    _condition.wait(lock, [this] { return !_queue.empty(); });

    T message = std::move(_queue.back());
    _queue.pop_back();

    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _queue.push_back(std::move(msg));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    while (true) {
        auto currentPhase = _messageQueue.receive();
        if (currentPhase == TrafficLightPhase::green) {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    std::random_device seeder;  
    std::mt19937 engine(seeder());
    std::uniform_int_distribution<int> dist(4, 6);
    int cycleDuration = dist(engine);

    auto lastUpdate = std::chrono::system_clock::now();

    while (true) {
        auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastUpdate).count();

        if (timeSinceLastUpdate >= cycleDuration) {
            _currentPhase = _currentPhase == TrafficLightPhase::green ? TrafficLightPhase::red : TrafficLightPhase::green;

            auto message = _currentPhase;
            auto future = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_messageQueue, std::move(message));
            
            future.wait();

            lastUpdate = std::chrono::system_clock::now();
            cycleDuration = dist(engine);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
