# Akiscode RTLCPP Library

`librtlcpp` is a C++11 library that provides many of the standard containers and utilities but does so in a way that is suitable for low-latency/real-time applications.

Specifically it utilizes the real-time memory allocator found in `librtl` to provide classes that can allocate memory in a time-bounded way.  Additionally, this entire library does not utilize exceptions or RTTI, meaning that applications may feel free to disable these "features".

Because exceptions are forbidden, copy constructors are deleted on most if not all containers since those may throw.  Additionally, all containers take the real time allocator as a template parameter.  __It is recommended that all containers use the same allocator as different template parameter types are classified as different types by the C++ type system.__

Instead, prefer any of the following:
1. Moving the containers (i.e. `std::move`)
2. Using references/pointers
3. Writing your own copy functions

Some classes (such as `rtl::vector`) provide their own copy functions since it is such a common operation.

*These containers are not guaranteed to work 1:1 with their standard library counterparts due to the need to disable/change functionality for the real time requirements.*


## License

This code is governed under the repository LICENSE file provided unless Akiscode has provided alternative terms via specific written authorization.
