// Deliberate bug: overflows a signed integer. UndefinedBehaviorSanitizer must stop this program.
#include <limits>

int main() {
    volatile int value = std::numeric_limits<int>::max();
    value = value + 1;
    return value < 0 ? 0 : 1;
}
