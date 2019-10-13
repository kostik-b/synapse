#include <ConfigManager.h>

#define BOOST_TEST_MODULE MyCircularBufferTest

#include <boost/test/included/unit_test.hpp>
#include <boost/circular_buffer.hpp>

BOOST_AUTO_TEST_CASE( CircularBufferTest )
{
    typedef boost::circular_buffer<double> CircularBuffer;
    CircularBuffer testBuffer(4);


    testBuffer.push_back (1.0);
    testBuffer.push_back (2.0);
    testBuffer.push_back (3.0);
    testBuffer.push_back (4.0);
    testBuffer.push_back (5.0);
    testBuffer.push_back (6.0);

    CircularBuffer::iterator iter = testBuffer.begin ();
    CircularBuffer::iterator end  = testBuffer.end ();

    int counter = 0;
    while (iter != end)
    {
        printf("element #%d, value is %f\n", counter, *iter);
        ++iter;
        ++counter;
    }

    BOOST_CHECK (true);
}
