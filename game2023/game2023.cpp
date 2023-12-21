#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

std::string _invasion;
std::mutex _invasion_mutex;

std::string get_invasion()
{
  std::lock_guard mut{_invasion_mutex};
  return _invasion;
}

std::string _shots;
std::mutex _shots_mutex;

char first_shot()
{
  std::lock_guard mut{_shots_mutex};
  return _shots.empty() ? ' ' : _shots.front();
}

class Char_generator
{
  std::random_device m_rd; // obtain a random number from hardware
  mutable std::mt19937 m_gen{m_rd()}; // seed the generator
  std::uniform_int_distribution<> m_distr{'A', 'Z'}; // define the range
public:
  char next() const
  {
    return static_cast<char>(m_distr(m_gen));
  }
};

static const std::string chars{"WISHINGYOUAMERRYCHRISTMASANDAHAPPYNEWYEAR"};
static const auto chars_begin = chars.cbegin();
static const auto chars_end = chars.cend();
static const size_t invasion_complete = chars.size();

class Char_generator_xmas
{
  std::string::const_iterator ichar{chars_begin};
public:
  char next()
  {
    const auto c = *ichar;
    if (++ichar == chars_end)
      ichar = chars_begin;
    return c;
  }
};

Char_generator_xmas _char_gen;

std::atomic<char> _target{Char_generator{}.next()};

std::atomic<bool> is_invasion_complete{false};

std::atomic<size_t> score{0};

void invade()
{
  auto delay = 100ms;

  while (!is_invasion_complete)
  {
    const auto next_char = _char_gen.next();

    {
      std::lock_guard mut{_invasion_mutex};
      _invasion.push_back(next_char);

    }

    std::this_thread::sleep_for(delay);

    if (delay > 10ms && score % 100 == 0)
      delay -= 10ms;
  }
}

void report()
{
  while (!is_invasion_complete)
  {
    const auto curr_invasion = get_invasion();
    const auto invasion_size = curr_invasion.size();

    if (invasion_size != 0)
    {
      const auto num_spaces = (invasion_complete - invasion_size)/2;
      std::cout << _target.load() << ':' << std::string(num_spaces, ' ') << curr_invasion << "\n";
    }

    if (invasion_size >= invasion_complete)
      is_invasion_complete = true;
    ++score;

    std::this_thread::sleep_for(7ms);
  }
}

bool contains(const std::string& text, const char c)
{
  return std::find(text.cbegin(), text.cend(), c) != text.cend();
}

void defend()
{
  while (!is_invasion_complete)
  {
    std::this_thread::sleep_for(8ms);

    const auto curr_invasion = get_invasion();

    auto curr_target = _target.load();

    if (contains(curr_invasion, curr_target))
    {
      std::lock_guard mut{_shots_mutex};
      _shots.push_back(_target);
      continue;
    }

    ++curr_target;
    if (curr_target > 'Z')
      curr_target = 'A';
    _target = curr_target;
  }
}

void resolve()
{
  static constexpr auto hit = '.';

  while (!is_invasion_complete)
  {
    std::string curr_shots;
    {
      std::lock_guard mut{_shots_mutex};
      std::swap(_shots, curr_shots);
    }

    {
      std::lock_guard mut{_invasion_mutex};

      for (const auto shot: curr_shots)
        if (const auto invader = std::find(_invasion.begin(), _invasion.end(), shot);
            invader != _invasion.end())
          *invader = hit;
    }

    std::this_thread::sleep_for(9ms);

    {
      std::lock_guard mut{_invasion_mutex};
      _invasion.erase(std::remove(_invasion.begin(), _invasion.end(), hit), _invasion.end());
    }
  }
}

int main()
{
  std::cout << "\n";

  std::vector<std::thread> threads;

  threads.emplace_back(invade);
  threads.emplace_back(defend);
  threads.emplace_back(resolve);
  threads.emplace_back(report);

  for (auto& thread: threads)
    thread.join();
}
