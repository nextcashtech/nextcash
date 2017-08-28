#include "arcmist/base/string.hpp"
#include "arcmist/io/buffer.hpp"
#include "arcmist/crypto/digest.hpp"


int main(int pArgumentCount, char **pArguments)
{
    int failed = 0;

    if(!ArcMist::String::test())
        failed++;

    if(!ArcMist::Buffer::test())
        failed++;

    if(!ArcMist::Digest::test())
        failed++;

    if(failed)
        return 1;
    else
        return 0;
}
