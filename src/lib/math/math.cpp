#include <math/math.h>

namespace math
{
    //TODO: Fix this trash
    void integrate(double interval, double *result)
    {
        double precision    = 10000;
        double dx           = interval/precision;
        double iterator     = 0.0;

        for (uint64_t i = 0; i < (uint64_t)precision; i++)
        {
            *result     += (iterator*iterator)*dx;
            iterator    += dx;
        }
    }

    //  In radians
    double sin(double theta)
    {
        if(theta < 0.0)                 return -1*sin(theta);

        theta = fmod(theta, 2*PI);

        if(PI < theta&&theta < (2*PI))  return -1*sin(theta - PI);
        if(theta > (PI/2))              return sin(PI - theta);

        return double(theta - (1/6.0)*pow(theta, 3) + (1/120.0)*pow(theta, 5) - (1/5040.0)*pow(theta, 7) );
    }
    double cos(double theta)
    {
        return sin(theta + (PI/2));
    }

    double LogisticMap(double r, double x0, UINT64 Iterations)
    {
        double Result = x0;
        for(UINT64 i = 0; i < Iterations; i++) Result = r*Result*(1-Result);
        return Result;
    }
}