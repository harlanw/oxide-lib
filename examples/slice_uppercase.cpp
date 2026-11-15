#include <oxide/c_str.hpp>
#include <oxide/io.hpp>
#include <oxide/string.hpp>

int main(int argc, char* argv[]) {
    char sample[] = "Hello, oxide!";
    char* input = argc > 1 ? argv[1] : sample;

    auto str = oxide::CStr::from_ptr(input).unwrap();
    auto uppercase = str.as_str().to_uppercase().collect<oxide::String>();

    println("{}", uppercase);

    return 0;
}
