// Deliberate bug: loses the only pointer to 64 bytes. LeakSanitizer must report them when the program ends.
#include <cstdlib>

// Volatile, so the compiler keeps both the allocation and the lost pointer.
void* volatile lostBlock = nullptr;

int main() {
    lostBlock = std::malloc(64);
    lostBlock = nullptr;
    return 0;
}
