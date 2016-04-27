#ifndef VARCO_CONCURRENT_HPP
#define VARCO_CONCURRENT_HPP

#include <algorithm>
#include <thread>
#include <type_traits>
#include <tuple>
#include <functional>
#include <mutex>
#include <future>
#include <map>

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
                                       const Reduce& reduce, const size_t maps_per_threads_hint = 20U) 
  {
    static_assert(
      prototype_traits<Map>::arity == 1 &&
      std::is_same<
        typename std::remove_reference<typename prototype_traits<Map>::return_type>::type,
        Accumulator
      >::value &&
        (std::is_same<
          typename std::remove_cv<typename std::remove_reference<typename prototype_traits<Map>::template param_type<0>::type>::type>::type,
          typename Sequence::value_type
        >::value || std::is_same<
          typename std::remove_reference<typename prototype_traits<Map>::template param_type<0>::type>::type,
          typename Sequence::value_type
        >::value),
      "Map intermediate / input type doesn't match the expected accumulator / input value"
      );

    static_assert(
      prototype_traits<Reduce>::arity == 2 &&
      std::is_same<
        typename std::remove_reference<typename prototype_traits<Reduce>::template param_type<0>::type>::type,
        Accumulator
      >::value &&
        (std::is_same<
          typename std::remove_cv<typename std::remove_reference<typename prototype_traits<Reduce>::template param_type<1>::type>::type>::type,
          Accumulator
        >::value || std::is_same<
          typename std::remove_reference<typename prototype_traits<Reduce>::template param_type<1>::type>::type,
          Accumulator
        >::value),
      "Reduce parameters don't match / incompatible with accumulator type"
      );
   
    using SequenceIterator = typename Sequence::const_iterator;
    static_assert(
      std::is_same<
        typename std::iterator_traits<SequenceIterator>::iterator_category,
        std::random_access_iterator_tag
      >::value,
      "Sequence hasn't random access iterators"
      );
    using AccumulatorIterator = typename Sequence::const_iterator;
    static_assert(
      std::is_same<
        typename std::iterator_traits<AccumulatorIterator>::iterator_category,
        std::random_access_iterator_tag
      >::value,
      "Accumulator hasn't random access iterators"
      );

    if (sequence.size() == 0)
      return Accumulator{};

    auto numThreads = std::max(1U, std::thread::hardware_concurrency());
    size_t minMapsPerThread = maps_per_threads_hint;

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

    std::mutex barrier_mutex;

    Accumulator result;
    std::map<size_t, Accumulator> threads_partials_map;
    std::vector<std::promise<void>> threads_promises;
    std::vector<std::future<void>> threads_futures;

    auto threadDispatcher = [&](size_t threadId, size_t start, size_t end) 
    {
      typename prototype_traits<Map>::return_type thread_partials;
      for (size_t i = start; i < end; ++i) {
        auto intermediate = map(sequence[i]);
        reduce(thread_partials, intermediate);
      }

      // ~-~-~-~-~-~-~-~-~ Sync barrier ~-~-~-~-~-~-~-~-~
      {
        std::unique_lock<std::mutex> lock(barrier_mutex);
        threads_partials_map.emplace(threadId, std::move(thread_partials));
        threads_promises[threadId].set_value();
      }      
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < numThreads; ++i) {
      size_t start = i * mapsPerThread;
      size_t end;
      if (numThreads == 1)
        end = sequence.size();
      else
        end = std::min(sequence.size(), start + mapsPerThread);
      {
        std::unique_lock<std::mutex> lock(barrier_mutex);
        threads_promises.emplace_back();
        threads_futures.emplace_back(threads_promises.back().get_future());
        threads.emplace_back(threadDispatcher, i, start, end);
      }
    }

    for (auto& future : threads_futures)
      future.wait();

    for (size_t i = 0; i < numThreads; ++i) {
      std::copy(threads_partials_map[i].begin(), threads_partials_map[i].end(),
                std::back_inserter(result)); 
    }

    for (auto& thread : threads)
      thread.join();

    return result;
  }

}


#endif // VARCO_CONCURRENT_HPP
