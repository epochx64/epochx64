#ifndef MATH_H
#define MATH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace math
{
    #define PI 3.14159265357
    /***
     * Only use floats or doubles
     * @tparam T
     * @param base
     * @param val
     * @return
     */
    template<class T>
    T log(T base, T val)
    {

    }

    /***
     * Integer log
     * @tparam T
     * @param base
     * @param val
     * @return
     */
    template<class T>
    T ilog(T base, T val)
    {
        T counter = 0;
        auto notref = (uint32_t)val;

        while( notref != 0)
        {
            notref /= base;
            counter++;
        }

        return counter;
    }

    /***
     * Use floats
     * @tparam T
     * @param val
     * @return
     */
    template<class T>
    int ceil(T val)
    {

    }

    /***
     * Use floats
     * @tparam T
     * @param val
     * @return
     */
    template<class T>
    int floor(T val)
    {

    }

    /*
     * Primitive pow() function, only allows for integer
     * exponents, but soon will be revamped to accept double
     * type exponent
     */
    template<class T>
    T pow(T base, uint64_t exponent)
    {
        T result = (T)1.0;

        for(uint64_t i = 0; i < exponent; i++) result *= base;

        return result;
    }

    void integrate(double interval, double *result);

    double sin(double theta);
    double cos(double theta);

    //  Floating point modulus
    inline double fmod(double val, double divisor)
    {
        auto n_divisors = (uint64_t)(val / divisor);
        return val - (double)(n_divisors*divisor);
    }

    template<class T>
    T abs(T val)
    {
        if (val < (T)0.0) return (T)(-1*val);
        return val;
    }
}

#endif
