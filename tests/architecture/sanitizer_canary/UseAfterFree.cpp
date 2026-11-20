// Deliberate bug: reads memory after it was released. AddressSanitizer must stop this program.
int main() {
    int* const value = new int(42);
    delete value;
    const volatile int read_after_free = *value;
    return read_after_free == 42 ? 0 : 1;
}
