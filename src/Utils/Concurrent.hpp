#ifndef VARCO_CONCURRENT_HPP
#define VARCO_CONCURRENT_HPP

#include <algorithm>
#include <thread>
#include <type_traits>
#include <tuple>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <sstream> // DEBUG

namespace varco {

  namespace {

    template <typename T>
    struct prototype_traits;

    template <typename Ret, typename... Args>
    struct prototype_traits < std::function<Ret(Args...)> > :
      public prototype_traits<Ret(Args...)>
    {};

    template <typename Ret, typename... Args>
    struct prototype_traits<Ret(Args...)> {

      constexpr static size_t arity = sizeof...(Args);

      using return_type = Ret;

      template <size_t i>
      struct param_type {
        using type = typename std::tuple_element<i, std::tuple<Args...>>::type;        
      };
    };
  }

  template <
    typename Accumulator,
    typename Sequence,
    typename Map,
    typename Reduce
  >
  Accumulator blockingOrderedMapReduce(const Sequence& sequence, const Map& map, 
                                       const Reduce& reduce) 
  {
    static_assert(
      prototype_traits<Map>::arity == 1 &&
      std::is_same<
        std::remove_reference<prototype_traits<Map>::return_type>::type,
        Accumulator
      >::value &&
        (std::is_same<
          std::remove_cv<std::remove_reference<prototype_traits<Map>::param_type<0>::type>::type>::type,
          Sequence::value_type
        >::value || std::is_same<
          std::remove_reference<prototype_traits<Map>::param_type<0>::type>::type,
          Sequence::value_type
        >::value),
      "Map intermediate / input type doesn't match the expected accumulator / input value"
      );

    static_assert(
      prototype_traits<Reduce>::arity == 2 &&
      std::is_same<
        std::remove_reference<prototype_traits<Reduce>::param_type<0>::type>::type,
        Accumulator
      >::value &&
        (std::is_same<
          std::remove_cv<std::remove_reference<prototype_traits<Reduce>::param_type<1>::type>::type>::type,
          Accumulator
        >::value || std::is_same<
          std::remove_reference<prototype_traits<Reduce>::param_type<1>::type>::type,
          Accumulator
        >::value),
      "Reduce parameters don't match / incompatible with accumulator type"
      );
   
    using Iterator = typename Sequence::const_iterator;

    if (sequence.size() == 0)
      return Accumulator{};

    auto numThreads = std::max(1U, std::thread::hardware_concurrency());
    size_t minMapsPerThread = 5U;

    size_t mapsPerThread;
    while(true) {
      mapsPerThread = static_cast<size_t>(
        std::ceil(sequence.size() / static_cast<double>(numThreads))
        );
      if (mapsPerThread < minMapsPerThread) {
        numThreads = std::max(numThreads / 2, 1U);
        if (numThreads > 1)
          continue;
      }
      break;
    }

    std::mutex mtx;
    std::condition_variable cv;
    std::mutex mtx_reduction;
    std::condition_variable cv_reduction;
    std::atomic_ullong syncIndex;
    syncIndex.store(0, std::memory_order_relaxed);
    std::atomic_bool syncFlag = false;
    bool reductionFlag = false;

    prototype_traits<Map>::return_type result;

    auto threadDispatcher = [&](size_t threadId, size_t start, size_t end) {
      prototype_traits<Map>::return_type thread_partials;
      for (size_t i = start; i < end; ++i) {
        auto intermediate = map(sequence[i]);
        reduce(thread_partials, intermediate);
      }

      // Sync barrier
      {
        std::unique_lock<std::mutex> lck(mtx);
        ++syncIndex;
        
        std::stringstream ss;
        ss << "Thread " << threadId << " just finished its first phase!\n";
        OutputDebugString(ss.str().c_str());

        cv.notify_all();
      }

      std::unique_lock<std::mutex> lck(mtx);
      bool allThreadsSyncd = syncFlag.load(std::memory_order_relaxed);
      size_t currentIndex = syncIndex.load(std::memory_order_relaxed);
      while (!(allThreadsSyncd && currentIndex == threadId)) {
        cv.wait(lck, [&]() {
          allThreadsSyncd = syncFlag.load(std::memory_order_relaxed);
          currentIndex = syncIndex.load(std::memory_order_relaxed);
          return allThreadsSyncd && currentIndex == threadId;
        });
        // if (condition_is_false) {} // Spurious wakeup
      }

      std::stringstream ss;
      ss << "Thread " << threadId << " woken up, now reducing!\n";
      OutputDebugString(ss.str().c_str());

      // Main reduction
      reduce(result, thread_partials);
      std::unique_lock<std::mutex> lck_red(mtx_reduction);
      reductionFlag = true;
      cv_reduction.notify_one();
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < numThreads; ++i) {
      size_t start = i * mapsPerThread;
      size_t end = std::min(sequence.size(), start + mapsPerThread);
      threads.emplace_back(threadDispatcher, i, start, end);      
    }

    std::unique_lock<std::mutex> lck(mtx);
    size_t currentIndex = syncIndex.load(std::memory_order_relaxed);
    while (currentIndex < numThreads) {
      cv.wait(lck, [&]() {
        currentIndex = syncIndex.load(std::memory_order_relaxed);
        return currentIndex == numThreads;
      });
      // if (currentIndex < numThreads) {} // Spurious wakeup
    }

    std::stringstream ss;
    ss << "[MAIN] ALL THREADS HAVE SYNC'D!!!\n";
    OutputDebugString(ss.str().c_str());

    // All threads sync'd
    syncFlag.store(true, std::memory_order_relaxed);
    lck.unlock();
    for (size_t i = 0; i < numThreads; ++i) {
      syncIndex.store(i);
      cv.notify_all();
      std::unique_lock<std::mutex> lck_red(mtx_reduction);
      while (!reductionFlag) {
        cv_reduction.wait(lck_red, [&]() {
          return reductionFlag;
        });
        // if (!reductionFlag) {} // Spurious wakeup
      }
      reductionFlag = false;
    }

    ss.str("");
    ss << "[MAIN] ALL THREADS HAVE REDUCED!!!\n";
    OutputDebugString(ss.str().c_str());
    
    for (auto& thread : threads)
      thread.join();

    return result;
  }


}


#endif // VARCO_CONCURRENT_HPP
