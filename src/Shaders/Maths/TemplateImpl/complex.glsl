
// #ifndef COMPLEX_TEMPLATE_scalar_t 
// #define COMPLEX_TEMPLATE_scalar_t float
// #endif

// #ifndef COMPLEX_TEMPLATE_real_t
// #define COMPLEX_TEMPLATE_real_t float
// #endif

// #ifndef COMPLEX_TEMPLATE_CONTINUOUS
// #define COMPLEX_TEMPLATE_CONTINUOUS 1
// #endif


COMPLEX_TEMPLATE_scalar_t Real(COMPLEX_TEMPLATE_complex_t z)
{
	return z.x;
}

COMPLEX_TEMPLATE_scalar_t Imaginary(COMPLEX_TEMPLATE_complex_t z)
{
	return z.y;
}

COMPLEX_TEMPLATE_unsigned_scalar_t Modulus2(COMPLEX_TEMPLATE_complex_t z)
{
	return length2(z);
}

COMPLEX_TEMPLATE_real_t Modulus(COMPLEX_TEMPLATE_complex_t z)
{
	return length(z);
}

// Returns in [-pi, pi]
COMPLEX_TEMPLATE_real_t Argument(COMPLEX_TEMPLATE_complex_t z)
{
	return atan(Imaginary(z), Real(z));
}

COMPLEX_TEMPLATE_complex_t Conjugate(COMPLEX_TEMPLATE_complex_t z)
{
	z.y = -z.y;
	return z;
}

#if COMPLEX_TEMPLATE_CONTINUOUS

COMPLEX_TEMPLATE_complex_t Normalize(COMPLEX_TEMPLATE_complex_t z)
{
	return normalize(z);
}

#endif

COMPLEX_TEMPLATE_complex_t complex_add(COMPLEX_TEMPLATE_complex_t a, COMPLEX_TEMPLATE_complex_t b)
{
	return a + b;
}

COMPLEX_TEMPLATE_complex_t cx_add(COMPLEX_TEMPLATE_complex_t a, COMPLEX_TEMPLATE_complex_t b)
{
	return complex_add(a, b);
}

COMPLEX_TEMPLATE_complex_t complex_sub(COMPLEX_TEMPLATE_complex_t a, COMPLEX_TEMPLATE_complex_t b)
{
	return a - b;
}

COMPLEX_TEMPLATE_complex_t cx_sub(COMPLEX_TEMPLATE_complex_t a, COMPLEX_TEMPLATE_complex_t b)
{
	return complex_sub(a, b);
}

COMPLEX_TEMPLATE_complex_t complex_mul(COMPLEX_TEMPLATE_complex_t a, COMPLEX_TEMPLATE_complex_t b)
{
	COMPLEX_TEMPLATE_complex_t res;
	res.x = Real(a) * Real(b) - Imaginary(a) * Imaginary(b);
	res.y = Real(a) * Imaginary(b) + Imaginary(a) * Real(b);
	return res;
}

COMPLEX_TEMPLATE_complex_t cx_mul(COMPLEX_TEMPLATE_complex_t a, COMPLEX_TEMPLATE_complex_t b)
{
	return complex_mul(a, b);
}

COMPLEX_TEMPLATE_complex_t complex_rcp(COMPLEX_TEMPLATE_complex_t z)
{
	COMPLEX_TEMPLATE_complex_t res;
	const COMPLEX_TEMPLATE_scalar_t m = COMPLEX_TEMPLATE_scalar_t(Modulus2(z));
	res.x = Real(z) / m;
	res.y = -Imaginary(z) / m;
	return res;
}

COMPLEX_TEMPLATE_complex_t cx_rcp(COMPLEX_TEMPLATE_complex_t z)
{
	return complex_rcp(z);
}

COMPLEX_TEMPLATE_complex_t complex_div(COMPLEX_TEMPLATE_complex_t num, COMPLEX_TEMPLATE_complex_t div)
{
	return complex_mul(num, complex_rcp(div));
}

COMPLEX_TEMPLATE_complex_t cx_div(COMPLEX_TEMPLATE_complex_t a, COMPLEX_TEMPLATE_complex_t b)
{
	return complex_div(a, b);
}

#if COMPLEX_TEMPLATE_CONTINUOUS

struct COMPLEX_TEMPLATE_polar_complex_t
{
	COMPLEX_TEMPLATE_scalar_t rho;
	COMPLEX_TEMPLATE_scalar_t phi;
};

COMPLEX_TEMPLATE_polar_complex_t ConvertToPolarComplex(COMPLEX_TEMPLATE_complex_t z)
{
	COMPLEX_TEMPLATE_polar_complex_t res;
	res.rho = Modulus(z);
	res.phi = Argument(z);
	return res;
}

COMPLEX_TEMPLATE_polar_complex_t Normalize(COMPLEX_TEMPLATE_polar_complex_t z)
{
	z.rho = 1;
	return z;
}

COMPLEX_TEMPLATE_scalar_t Modulus2(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return z.rho * z.rho;
}

COMPLEX_TEMPLATE_scalar_t Modulus(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return z.rho;
}

COMPLEX_TEMPLATE_scalar_t Argument(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return z.phi;
}

COMPLEX_TEMPLATE_scalar_t Real(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return z.rho * cos(z.phi);
}

COMPLEX_TEMPLATE_scalar_t Imaginary(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return z.rho * sin(z.phi);
}

// mod z.phi to be in [-PI, PI]
COMPLEX_TEMPLATE_polar_complex_t SanitizeArg(COMPLEX_TEMPLATE_polar_complex_t z)
{
	float p = z.phi + PI;
	p = mod(p, TWO_PI);
	z.phi = p - PI;
	return z;
}

COMPLEX_TEMPLATE_complex_t ConvertToCarthesianComplex(COMPLEX_TEMPLATE_polar_complex_t z)
{
	COMPLEX_TEMPLATE_complex_t res;
	res.x = Real(z);
	res.y = Imaginary(z);
	return res;
}

COMPLEX_TEMPLATE_complex_t ConvertToComplex(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return ConvertToCarthesianComplex(z);
}

COMPLEX_TEMPLATE_polar_complex_t Conjugate(COMPLEX_TEMPLATE_polar_complex_t z)
{
	z.phi = -z.phi;
	return z;
}

COMPLEX_TEMPLATE_polar_complex_t complex_rcp(COMPLEX_TEMPLATE_polar_complex_t z)
{
	COMPLEX_TEMPLATE_polar_complex_t res;
	res.rho = rcp(z.rho);
	res.phi = -z.phi;
	return res;
}

COMPLEX_TEMPLATE_polar_complex_t cx_rcp(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return complex_rcp(z);
}

// Warning: does not sanitize the Arg!
COMPLEX_TEMPLATE_polar_complex_t complex_mul(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	COMPLEX_TEMPLATE_polar_complex_t res;
	res.rho = a.rho * b.rho;
	res.phi = a.phi + b.phi;
	return res;
}

COMPLEX_TEMPLATE_polar_complex_t cx_mul(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return complex_mul(a, b);
}

// Warning: does not sanitize the Arg!
COMPLEX_TEMPLATE_polar_complex_t complex_div(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	COMPLEX_TEMPLATE_polar_complex_t res;
	res.rho = a.rho / b.rho;
	res.phi = a.phi - b.phi;
	return res;
}

COMPLEX_TEMPLATE_polar_complex_t cx_div(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return complex_div(a, b);
}


COMPLEX_TEMPLATE_complex_t complex_convert_add(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	// Don't see a better method
	COMPLEX_TEMPLATE_complex_t res = ConvertToComplex(a) + ConvertToComplex(b);
	return res;
}

COMPLEX_TEMPLATE_polar_complex_t complex_add(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return ConvertToPolarComplex(complex_convert_add(a, b));
}

COMPLEX_TEMPLATE_complex_t cx_convert_add(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return complex_convert_add(a, b);
}

COMPLEX_TEMPLATE_polar_complex_t cx_add(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return complex_add(a, b);
}


COMPLEX_TEMPLATE_complex_t complex_convert_sub(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	// Don't see a better method
	COMPLEX_TEMPLATE_complex_t res = ConvertToComplex(a) - ConvertToComplex(b);
	return res;
}

COMPLEX_TEMPLATE_polar_complex_t complex_sub(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return ConvertToPolarComplex(complex_convert_sub(a, b));
}

COMPLEX_TEMPLATE_complex_t cx_convert_sub(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return complex_convert_sub(a, b);
}

COMPLEX_TEMPLATE_polar_complex_t cx_sub(COMPLEX_TEMPLATE_polar_complex_t a, COMPLEX_TEMPLATE_polar_complex_t b)
{
	return complex_sub(a, b);
}


COMPLEX_TEMPLATE_complex_t complex_exp(COMPLEX_TEMPLATE_complex_t z)
{
	return exp(Real(z)) * COMPLEX_TEMPLATE_complex_t(cos(Imaginary(z)), sin(Imaginary(z)));
}

COMPLEX_TEMPLATE_polar_complex_t complex_exp(COMPLEX_TEMPLATE_polar_complex_t z)
{
	COMPLEX_TEMPLATE_polar_complex_t res;
	res.rho = exp(z.rho * cos(z.phi));
	res.phi = z.rho * sin(z.phi);
	return res;
}

COMPLEX_TEMPLATE_complex_t cx_exp(COMPLEX_TEMPLATE_complex_t z)
{
	return complex_exp(z);
}

COMPLEX_TEMPLATE_polar_complex_t cx_exp(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return complex_exp(z);
}

COMPLEX_TEMPLATE_complex_t complex_log(COMPLEX_TEMPLATE_complex_t z)
{
	COMPLEX_TEMPLATE_complex_t res;
	res.x = log(Modulus(z));
	res.y = Argument(z);
	return res;
}

COMPLEX_TEMPLATE_complex_t complex_convert_log(COMPLEX_TEMPLATE_polar_complex_t z)
{
	COMPLEX_TEMPLATE_complex_t res;
	res.x = log(z.rho);
	res.y = z.phi;
	return res;
}

COMPLEX_TEMPLATE_polar_complex_t complex_log(COMPLEX_TEMPLATE_polar_complex_t z)
{
	COMPLEX_TEMPLATE_complex_t c = complex_convert_log(z);
	return ConvertToPolarComplex(c);
}

COMPLEX_TEMPLATE_complex_t cx_log(COMPLEX_TEMPLATE_complex_t z)
{
	return complex_log(z);
}

COMPLEX_TEMPLATE_complex_t cx_convert_log(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return complex_convert_log(z);
}

COMPLEX_TEMPLATE_polar_complex_t cx_log(COMPLEX_TEMPLATE_polar_complex_t z)
{
	return complex_log(z);
}

#endif

