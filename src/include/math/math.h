#ifndef _MATH_H
#define _MATH_H

#include <defs/int.h>

namespace math
{
    template<class T>
    class Vec2
    {
    public:
        Vec2(T x, T y)
        {
            X = x;
            Y = y;
        }

        T X;
        T Y;
    };

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

        while( val != 0)
        {
            val /= base;
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
        return 0;
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
        return 0;
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

    //  Useful for pseudo random numbers
    //  Veritasium, thx bro
    double LogisticMap(double r, double x0, UINT64 Iterations);

    //  Super basic, doesn't care about even-round, doesn't know about negatives
    template <class T> T RoundDouble(double x)
    {
        double RoundDown = (UINT64)x;
        double Decimal = x - RoundDown;

        if (Decimal < 0.5) return (T)RoundDown;
        else return (T)(RoundDown + 1);
    }
}

#endif
