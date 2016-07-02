#ifndef VARCO_INTERPOLATORS_HPP
#define VARCO_INTERPOLATORS_HPP

#include <chrono>

namespace varco {

  class InterpolationSequence {
  private:

  };
  
  class LinearInterpolator {
  public:
    LinearInterpolator(int startValue, int endValue, int milliseconds, bool cycle) :
      m_startValue(startValue),
      m_endValue(endValue),
      m_milliseconds(milliseconds),
      m_cycle(cycle)
    {
      m_begin = std::chrono::high_resolution_clock::now();
    }

    int getValue() {
      auto end = std::chrono::high_resolution_clock::now();
      int elapsedMs = 0;

      do {
        auto duration = end - m_begin;
        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        if (elapsedMs > m_milliseconds) {
          //
          // |----|----|----|--|
          // beg            p  end
          //
          // subtracts the segment (p;end) from end and assigns it to the begin.
          // Prevents overflow and
          m_begin = end - std::chrono::milliseconds{(static_cast<int>(elapsedMs / m_milliseconds) + 1) % elapsedMs};
          continue;
        }
        break;
      } while(true);

      if (!m_cycle && elapsedMs >= m_milliseconds)
        return m_endValue;

      //auto test = (static_cast<float>(m_milliseconds % ms) / static_cast<float>(m_milliseconds));
      //auto lol = (static_cast<float>(m_milliseconds % ms) / static_cast<float>(m_milliseconds)) * (m_endValue - m_startValue) + m_startValue;

      //printf("Returning %d\n", (static_cast<float>(m_milliseconds % ms) / static_cast<float>(m_milliseconds)) * (m_endValue - m_startValue) + m_startValue);
      return (static_cast<float>(elapsedMs) / static_cast<float>(m_milliseconds)) * (m_endValue - m_startValue) + m_startValue;
    }

  private:
    int   m_startValue;
    int   m_endValue;
    int   m_milliseconds;
    bool  m_cycle; // Whether we should be cycling the results or just stick with the endValue

    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
  };

}

#endif // VARCO_INTERPOLATORS_HPP
