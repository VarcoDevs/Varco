#ifndef VARCO_CONCURRENT_HPP
#define VARCO_CONCURRENT_HPP

#include <Document/Document.hpp>
#include <algorithm>
#include <cmath>
#include <vector>
#include <thread>
#include <memory>
#include <type_traits>
#include <tuple>
#include <functional>
#include <mutex>
#include <future>
#include <map>

namespace varco {

  // Two classes help to organize a document to be displayed and are set up by a document
  // recalculate operation:
  //
  // PhysicalLine {
  //   A physical line corresponds to a real file line (until a newline character)
  //   it should have 0 (only a newline) or more EditorLine
  //
  //   EditorLine {
  //     An editor line is a line for the editor, i.e. a line that might be the result
  //     of wrapping or be equivalent to a physical line. EditorLine stores the characters
  //   }
  // }

  struct EditorLine {
    EditorLine(std::string str);

    std::vector<char> m_characters;
  };

  struct PhysicalLine {
    PhysicalLine(EditorLine editorLine) {
      m_editorLines.emplace_back(std::move(editorLine));
    }
    PhysicalLine(std::vector<EditorLine>&& editorLines) {
      m_editorLines = std::forward<std::vector<EditorLine>>(editorLines);
    }
    PhysicalLine(const PhysicalLine&) = default;
    PhysicalLine() = default;

    std::vector<EditorLine> m_editorLines;
  };

  enum SyntaxHighlight { NONE, CPP };

  struct ThreadRequest { // A workload request for a thread

    std::mutex m_syncBarrier; // Sync barrier for threads of this thread request

    // Variables related to how the control renders lines
    SkScalar m_characterWidthPixels;
    SkScalar m_characterHeightPixels;
    int m_wrapWidthPixels;
    int m_maximumCharactersLine; // According to wrapWidth
    StyleDatabase m_styleDb;

    std::vector<std::string> m_plainTextLines;
    std::vector<PhysicalLine> m_physicalLines;

    size_t m_numThreads = 1;
    size_t m_linesPerThread;
    SkScalar m_totalBitmapHeight = 0;
    SkScalar m_maxBitmapWidth = 0;
    std::vector<std::promise<std::tuple<std::vector<PhysicalLine>, SkBitmap,
      SkScalar /* effective width */, SkScalar /* effective height */>>> m_partials;
    std::vector<std::future<std::tuple<std::vector<PhysicalLine>, SkBitmap,
      SkScalar /* effective width */, SkScalar /* effective height */>>> m_futures;

    std::function<void(size_t, std::shared_ptr<ThreadRequest>)> m_callback; // Work function
    std::function<void(std::shared_ptr<ThreadRequest>)> m_endCallback; // End function
  };

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


  class ThreadPool {
  public:
    ThreadPool(){
      m_workloadReady.resize(m_NThreads, false);
      for (size_t i = 0; i < m_NThreads; ++i)
        m_threads.emplace_back(&ThreadPool::threadMain, this, i);
      m_threadsIdle = m_NThreads;
    }
    ~ThreadPool() {
      m_sigterm = true;
      m_cv.notify_all();
      for (auto& thread : m_threads) {
        if (thread.joinable())
          thread.join();
      }
    }

    void setFinishedCV(std::condition_variable *cv) {
      m_allWorkFinishedCV.reset(cv);
    }
    /*void setCallback(std::function<void(size_t, std::shared_ptr<ThreadRequest>)> callback) {
      m_callback = callback;
    }*/
    /*void setEndCallback(std::function<void()> end_callback) {
      m_end_callback = end_callback;
    }*/

    void addRequest(std::shared_ptr<ThreadRequest> request) {
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pendingRequests.push_back(request);

        // Start working
        start();
      }
      m_cv.notify_all(); // Must be done without lock
    }

    const size_t m_NThreads = 15; // Number of threads of the threadpool

  private:
    void threadMain(size_t threadIdx) {
      while (!m_sigterm) {
        {
          std::unique_lock<std::mutex> lock(m_mutex);
          while (!m_sigterm && !m_workloadReady[threadIdx]) {
            m_cv.wait(lock);
          }
        }
        if (m_sigterm)
          return;

        m_currentRequest->m_callback(threadIdx, m_currentRequest);

        {
          std::unique_lock<std::mutex> lock(m_mutex);
          m_workloadReady[threadIdx] = false;
          ++m_threadsIdle;

          if (m_threadsIdle == m_NThreads) {
            m_currentRequest->m_endCallback(m_currentRequest);
            m_currentRequest.reset();
            if (m_allWorkFinishedCV)
              m_allWorkFinishedCV->notify_all(); // Signal ThreadPool owner we're done

            // Start another workload unit (if there's any)
            start();
          }
        }
        m_cv.notify_all(); // Must be done without lock
      }
    }

    // For internal use - whoever calls these must have acquired a valid lock

    void start() { // Dispatch a pending request if the threadpool is idle
      if (!idle())
        return; // We're already doing something
      {
        if (m_pendingRequests.empty())
          return;

        // Grab the latest pending request and start working
        m_currentRequest = m_pendingRequests.back();
        m_pendingRequests.clear();

        std::fill(m_workloadReady.begin(), m_workloadReady.end(), true);
        m_threadsIdle = 0;
      }      
    }
    bool idle() {
      return (m_threadsIdle == m_NThreads);
    }

    std::vector<std::thread> m_threads;    
    size_t m_threadsIdle = 0;
    bool m_sigterm = false;
    std::vector<bool> m_workloadReady;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::function<void(size_t, std::shared_ptr<ThreadRequest>)> m_callback;
    std::function<void()> m_end_callback;
    // A condition variable to signal all work has been done
    std::shared_ptr<std::condition_variable> m_allWorkFinishedCV;

    std::shared_ptr<ThreadRequest> m_currentRequest; // Working on this
    std::vector<std::shared_ptr<ThreadRequest>> m_pendingRequests;
  };

}


#endif // VARCO_CONCURRENT_HPP
