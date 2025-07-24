#include <chrono>
#include <conio.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <windows.h>

using std::size_t;
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using std::chrono::duration_cast;
using std::chrono::milliseconds;


std::string your_name {};
std::filesystem::path path {};
double total_income {};
double last_income {};
long long last_pos {};
constexpr std::string_view TARGET {"[CHAT] You got: $"};

enum class TimerMode{Stopwatch, Countdown};

class Timer{
public:
    Timer (TimerMode m) : mode(m) {}

    void toggle(){
        auto now {Clock::now()};
        if (is_running) end = now;
        else start = now - (end - start);         
        is_running = !is_running;
    }

    void reset(){
        is_running = false;
        start = end = {};
    }

    std::string timeFormat(int64_t ms) const{
        char buf [32];
        int64_t hr {ms / 3'600'000};
        int64_t min {(ms / 60'000) % 60};
        int64_t sec {(ms / 1000) % 60};
        std::snprintf(buf, sizeof(buf),"%3lld:%02lld:%02lld", hr, min, sec);
        return std::string(buf);
    }

    std::string getSeconds(int64_t ms) const{
        return std::to_string(ms / 1000);
    }

    int64_t getTimeMs(){        
        constexpr int64_t TIME_LIMIT {360'000 * 1000};

        auto now {is_running ? Clock::now() : end};
        int64_t duration {duration_cast<milliseconds>(now - start).count()};
        
        switch (mode){
            case TimerMode::Stopwatch:{ 
                if (duration > TIME_LIMIT){
                    is_running = false;
                    end = start + milliseconds(TIME_LIMIT);
                    return TIME_LIMIT;
                }
                return duration;
            }
            case TimerMode::Countdown:{         
                int64_t remaining {remaining_ms - duration};
                
                if (remaining <= 0){
                    is_running = false;
                    end = now;
                    return 0;
                }
                return remaining;
            }
        }
        return 0;
    }

    bool getIsRunning() const{ return is_running; }

private:
    TimerMode mode {};
    TimePoint start {};
    TimePoint end {};
    bool is_running {false};
    int64_t time_ms {};
    int64_t remaining_ms {};
};

std::string toComma(double num) {
    std::ostringstream oss {};
    oss << std::fixed << std::setprecision(2) << num;
    std::string s {oss.str()};

    std::string whole {s.substr(0, s.find('.'))};
    std::string dec {s.substr(s.find('.'))};

    int pos {static_cast<int>(whole.size()) - 3};
    for (;pos > 0; pos -= 3) whole.insert(static_cast<size_t>(pos), ",");
    
    return '$' + whole + dec;
}

std::vector<std::string> monitorLog(){
    std::ifstream file {path, std::ios::ate};
    if (!file) return {};

    if (last_pos == 0){
        last_pos = file.tellg();
        return {};
    }
    file.seekg(last_pos);
    std::vector<std::string> new_lines {};
    std::string line {};

    while (std::getline(file, line)){
        last_pos = file.tellg();
        if (line.find(TARGET) != std::string::npos){
            new_lines.push_back(line);
        }
    }
    return new_lines;
}

double extractAmount(Timer& t, const std::string& msg){  
    if (t.getIsRunning() == false) return {}; 
    size_t target_pos {msg.find(TARGET)};
    if (target_pos == std::string::npos) return {};

    try{
        std::string after_target {msg.substr(target_pos + TARGET.size())};
        return std::stod(after_target);
    }
    catch (const std::exception&){
        return {};
    }
}

double ratePerHr(Timer& t){
    int64_t elapsed_sec {t.getTimeMs() / 1000};
    if (elapsed_sec == 0) return {};
    return {(total_income / static_cast<double>(elapsed_sec)) * 3600};
}

void update(Timer& t){
    for (const auto& line : monitorLog()){
        double income {extractAmount(t, line)};
        if (income > 0){
            last_income = income;
            total_income += income;
        }
    }
}

void hInput(Timer& t){
    if (_kbhit()){
        int key {std::tolower(static_cast<unsigned char>(_getch()))};
        switch (key){
            case 27: std::exit(0);
            case 's': t.toggle(); break;
            case 'r': t.reset(); break;
            default: break;
        }
    }
}

void draw(Timer& t){
    char buf [256];
    constexpr const char* HEADER {
        "\033[?25l\033[0J\033[H\033[33m"
        "%s\n\033[36m"
        "Timer\t\033[32m%11s\033[36m\n"
        "Gained\t\033[32m%11s\033[36m\n"
        "Total\t\033[32m%11s\033[36m\n"
        "\033[32m%16s/hr"
    };

    while (true){
        update(t);
        hInput(t);
        std::snprintf(buf, sizeof(buf), HEADER,
        your_name.c_str(),
        t.timeFormat(t.getTimeMs()).c_str(),
        toComma(last_income).c_str(),
        toComma(total_income).c_str(),
        toComma(ratePerHr(t)).c_str());
        std::cout << buf << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

void consoleSetup(){
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oMode = 0, iMode = 0;

    if (GetConsoleMode(hOut, &oMode)) {
        oMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, oMode);
    }

    if (GetConsoleMode(hIn, &iMode)) {
        iMode &= ~(DWORD)ENABLE_MOUSE_INPUT;
        iMode &= ~(DWORD)ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hIn, iMode);
    }
}

int main(){
    consoleSetup();
    SetConsoleTitleA("(=ↀᆺↀ=)");
    std::cout << "\033[2J\033[3J\033[H";

    your_name = "~ ✦ Your Timer ✦ ~";
    path = "D:/MultiMC/instances/1.21.1/.minecraft/logs/latest.log";

    Timer t {TimerMode::Stopwatch};
    draw(t);

    return 0;
}