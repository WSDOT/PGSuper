Minimum Radius of Curvature {#minimum_radius_of_curvature}
==============================================================

# General
Curvature of an equation in rectangular coordinates, \f$ y = f(x) \f$

\f[
K = \frac{y''}{\left( 1 + y'^2 \right)^{\frac{3}{2}}}
\f]

Radius of curvature

\f[
R = \frac{1}{K}
\f]

\f[
R(x) = \frac{\left( 1 + y'^2 \right)^{\frac{3}{2}}}{y''}
\f]

# Parabolic Tendons
Parabolic tendons are modeled with an equation of the form

\f[
y(x) = Ax^{2} + Bx + C
\f]

\f[
y' = 2Ax + B
\f]

\f[
y'' = 2A
\f]

\f[
R(x) = \frac{1}{2A}\left[ 1 + (2Ax + B)^{2} \right]^{\frac{3}{2}}
\f]

Find the location of minimum R by taking its derivative, setting it equal to zero, and solving for x

\f[
R'(x) = 3\left[ 1 + (2Ax + B)^{2} \right]^{\frac{3}{2}}(2Ax + B) = 0
\f]

\f[
x = -\frac{B}{2A}
\f]

Substitute in the equation for R

\f[
R_{Min} = \frac{1}{2A}
\f]

# Linear Tendons
The curvature of a straight line is zero; therefore the radius of curvature is infinite. However, in PGSplice™ we can model tendons as a series of linear segments. The linear segments are assumed approximate a curve. The approximate radius of curvature is computed as follows:

Line Segment 1: \f$ \left( x_{1}, y_{1} \right) \rightarrow \left( x_{2}, y_{2} \right) \f$

Line Segment 2: \f$ \left( x_{2}, y_{2} \right) \rightarrow \left( x_{3}, y_{3} \right) \f$

Derivative of line segment 1 = Slope = \f$ m_{1} = \frac{y_{2} - y_{1}}{x_{2} - x_{1}} \f$

Derivative of line segment 2 = Slope = \f$ m_{2} = \frac{y_{3} - y_{2}}{x_{3} - x_{2}} \f$

\f[
\frac{dy}{dx} \approx \Delta m = \frac{m_{1} + m_{2}}{2}
\f]

\f[
\frac{d^{2}y}{dx^{2}} \approx \frac{\Delta m}{\Delta x} = \frac{\Delta m}{x_{2} - x_{1}}
\f]

Compute the first and second derivatives and solve for R.
