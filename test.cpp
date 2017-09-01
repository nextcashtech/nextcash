#include "arcmist/base/string.hpp"
#include "arcmist/base/log.hpp"
#include "arcmist/io/buffer.hpp"
#include "arcmist/crypto/digest.hpp"


int main(int pArgumentCount, char **pArguments)
{
    int failed = 0;

    ArcMist::Log::setLevel(ArcMist::Log::DEBUG);

    if(!ArcMist::String::test())
        failed++;

    if(!ArcMist::Buffer::test())
        failed++;

    if(!ArcMist::Digest::test())
        failed++;

    ArcMist::Log::add(ArcMist::Log::DEBUG, "Test", "Debug Color Test");
    ArcMist::Log::add(ArcMist::Log::VERBOSE, "Test", "Verbose Color Test");
    ArcMist::Log::add(ArcMist::Log::INFO, "Test", "Info Color Test");
    ArcMist::Log::add(ArcMist::Log::WARNING, "Test", "Warning Color Test");
    ArcMist::Log::add(ArcMist::Log::ERROR, "Test", "Error Color Test");

    if(failed)
        return 1;
    else
        return 0;
}
