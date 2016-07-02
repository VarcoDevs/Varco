#ifndef VARCO_INTERPOLATORS_HPP
#define VARCO_INTERPOLATORS_HPP

#include <chrono>
#include <vector>
#include <memory>

namespace varco {

  class InterpolationSequence;

  class InterpolatorBase { // Base interface for all interpolators
    friend class InterpolationSequence;
  protected:
    InterpolatorBase(int startValue, int endValue, int milliseconds) :
      m_startValue(startValue),
      m_endValue(endValue),
      m_milliseconds(milliseconds)
    {
      //m_begin = std::chrono::high_resolution_clock::now();
    }

    virtual int getValue(long long ms /* ms from startValue */) = 0;

    int   m_startValue;
    int   m_endValue;
    int   m_milliseconds;

    //std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
  };
  
  // A linear interpolator just linearly interpolates from startValue to endValue in 'milliseconds' ms,
  // if 'cycle' is set to true, it will cycle when called multiple times even after the deadline expired.
  // If set to false it will return endValue after the deadline has expired.
  class LinearInterpolator : public InterpolatorBase {
  public:
    LinearInterpolator(int startValue, int endValue, int milliseconds) :
      InterpolatorBase(startValue, endValue, milliseconds)
    {}

    int getValue(long long ms) override {
      if (ms > m_milliseconds)
        return -1; // Invalid

      return (static_cast<float>(ms) / static_cast<float>(m_milliseconds)) * (m_endValue - m_startValue) + m_startValue;
//      auto end = std::chrono::high_resolution_clock::now();
//      int elapsedMs = 0;

//      do {
//        auto duration = end - m_begin;
//        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

//        if (elapsedMs > m_milliseconds) {
//          //
//          // |----|----|----|--|
//          // beg            p  end
//          //
//          // subtracts the segment (p;end) from end and assigns it to the begin.
//          // Prevents overflow and
//          m_begin = end - std::chrono::milliseconds{(static_cast<int>(elapsedMs / m_milliseconds) + 1) % elapsedMs};
//          continue;
//        }
//        break;
//      } while(true);

//      return (static_cast<float>(elapsedMs) / static_cast<float>(m_milliseconds)) * (m_endValue - m_startValue) + m_startValue;
    }
  };

  // A constant interpolator keeps the same value for a specified amount of time
  class ConstantInterpolator : public InterpolatorBase {
  public:
    ConstantInterpolator(int value, int milliseconds) :
      InterpolatorBase(value, value, milliseconds)
    {}

    int getValue(long long) override {
      return m_startValue;
    }
  };

  class InterpolationSequence {
  public:

    void start() {
      m_begin = std::chrono::high_resolution_clock::now();
    }

    void addInterpolator(std::unique_ptr<InterpolatorBase> interpolator) {
      sequenceDurationMs += interpolator->m_milliseconds;
      m_sequence.emplace_back(std::move(interpolator));
      m_intervalMap.emplace(sequenceDurationMs, m_sequence.size() - 1);
    }

    //if (!m_cycle && elapsedMs >= m_milliseconds)
      //return m_endValue;

    void setCycle(bool cycle) {
      m_cycle = cycle;
    }

    // Called at any point in time, returns the right interpolator's value
    int getValue() {
      auto end = std::chrono::high_resolution_clock::now();
      long long elapsedMs = 0;

      auto duration = end - m_begin;
      elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

      if (elapsedMs > sequenceDurationMs) {
        // (beg;end) is elapsedMs and is longer than a single sequence duration
        //
        // |----|----|----|--|
        // beg            p  end
        //
        // Finds the segment (p;end) as delta and assigns begin to p.
        // Prevents overflow and ensures valid interpolation calculations.
        auto deltaMs = elapsedMs - (static_cast<int>(elapsedMs / sequenceDurationMs) * sequenceDurationMs);
        m_begin = end - std::chrono::milliseconds{deltaMs};
        elapsedMs = deltaMs;
      }

      if (elapsedMs > sequenceDurationMs)
        elapsedMs = sequenceDurationMs; // Should never happen

      // Detect the right interpolator to call
      auto it = m_intervalMap.lower_bound(elapsedMs); // Returns the interval that ends at elapsedMs or goes after it
      if (it == m_intervalMap.end()) // Should never happen
        return -1;

      // Calculate delta into the selected interval (i.e. the offset from its start)
      long long endPreviousInterval = 0;
      if (it != m_intervalMap.end() && it != m_intervalMap.begin()) {
        auto previous = it;
        --previous;
        // There's an interval before this one, grab its end value
        endPreviousInterval = previous->first;
      }
      long long msIntervalDelta = elapsedMs - endPreviousInterval;

      return m_sequence[it->second]->getValue(msIntervalDelta);

      //return (static_cast<float>(elapsedMs) / static_cast<float>(m_milliseconds)) * (m_endValue - m_startValue) + m_startValue;
    }

  private:
    std::vector<std::unique_ptr<InterpolatorBase>> m_sequence;
    std::map<long long /* end of a sequence in ms */, size_t /* index into m_sequence */> m_intervalMap;
    long long sequenceDurationMs = 0;
    bool  m_cycle; // Whether we should be cycling the results or just stick with the endValue
    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
  };

}

#endif // VARCO_INTERPOLATORS_HPP
