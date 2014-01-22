Specialization instead of plain inheritance?
Template specialization removes need for virtual functions (?).

```cpp
struct arch_x86 {};

template<typename arch_t>
class virtual_address_space_t;

template <>
class virtual_address_space_t<arch_x86>
{
    // Specialized for x86
};
```
----------------------------------------------------------------------

Try and have separate ABI and APIs for the kernel, much better for portability and writing new software.

----------------------------------------------------------------------

When you send multiple or big files to someone, they will be transparently compressed into a vxa archive with decoder
for recipient's architecture(s) embedded, and then transparently uncompressed at the receiving side.

----------------------------------------------------------------------