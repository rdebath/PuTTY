#include <stdio.h>
#include <math.h>

struct labcolour { double L, a, b; };
struct xyzcolour { double X, Y, Z; };

/*** function defs ***/
static inline
void deltaE2000( double *lab1, double *lab2, double *delta_e );

static inline
struct xyzcolour sRGB_to_xyz(int cR, int cG, int cB);
static inline
struct labcolour xyz_to_lab(struct xyzcolour c);

/*
* Returns rgb converted to xyzcolor.
*/

static inline
struct xyzcolour
sRGB_to_xyz(int cR, int cG, int cB)
{
  // Based on http://www.easyrgb.com/index.php?X=MATH&H=02
  double R = ( cR / 255.0 );
  double G = ( cG / 255.0 );
  double B = ( cB / 255.0 );

#define ALPHA 0.055
  if ( R > 0.04045 ) R = pow(( ( R + 0.055 ) / 1.055 ),2.4);
  else               R = R / 12.92;
  if ( G > 0.04045 ) G = pow(( ( G + 0.055 ) / 1.055 ),2.4);
  else               G = G / 12.92;
  if ( B > 0.04045 ) B = pow(( ( B + 0.055 ) / 1.055 ),2.4);
  else               B = B / 12.92;

  R *= 100;
  G *= 100;
  B *= 100;

#if 0
  // Observer. = 2°, Illuminant = D65
  // White point is not D65: 95.05,100.00,108.90
  double X = R * 0.4124 + G * 0.3576 + B * 0.1805;
  double Y = R * 0.2126 + G * 0.7152 + B * 0.0722;
  double Z = R * 0.0193 + G * 0.1192 + B * 0.9505;
#endif

#if 0
  // Observer. = 2°, Illuminant = D65
  // White point is not D65: 95.0456,100.0000,108.8754
  double X = R * 0.412453 + G * 0.357580 + B * 0.180423;
  double Y = R * 0.212671 + G * 0.715160 + B * 0.072169;
  double Z = R * 0.019334 + G * 0.119193 + B * 0.950227;
#endif

#if 1
  // Observer. = 2°, Illuminant = D65
  double X = R * 0.4124564 + G * 0.3575761 + B * 0.1804375;
  double Y = R * 0.2126729 + G * 0.7151522 + B * 0.0721750;
  double Z = R * 0.0193339 + G * 0.1191920 + B * 0.9503041;

  // 4DP rounding gives correct white point.
  // ie: #FFFFFF maps to D65 => 95.047,100.000,108.883
  X = roundf(X * 10000) / 10000;
  Y = roundf(Y * 10000) / 10000;
  Z = roundf(Z * 10000) / 10000;
#endif

  struct xyzcolour rv;
  rv.X = X;
  rv.Y = Y;
  rv.Z = Z;
  return rv;
}

/*
* Returns xyzcolour converted to labcolor.
*/
static inline
struct labcolour
xyz_to_lab(struct xyzcolour c)
{
  // Based on http://www.easyrgb.com/index.php?X=MATH&H=07
  double ref_X = 95.047; // Observer= 2°, Illuminant= D65
  double ref_Y = 100.000;
  double ref_Z = 108.883;

  double Y = c.Y / ref_Y;
  double Z = c.Z / ref_Z;
  double X = c.X / ref_X;
  if ( X > 0.008856 ) X = pow(X, 1.0/3.0);
  else                X = ( 7.787 * X ) + ( 16.0 / 116.0 );
  if ( Y > 0.008856 ) Y = pow(Y, 1.0/3.0);
  else                Y = ( 7.787 * Y ) + ( 16.0 / 116.0 );
  if ( Z > 0.008856 ) Z = pow(Z, 1.0/3.0);
  else                Z = ( 7.787 * Z ) + ( 16.0 / 116.0 );
  double L = ( 116.0 * Y ) - 16.0;
  double a = 500.0 * ( X - Y );
  double b = 200.0 * ( Y - Z );

  struct labcolour rv;
  rv.L = L;
  rv.a = a;
  rv.b = b;
  return rv;
}

#define pi 3.141592653589793238462643383279

/// This function from:
/// https://github.com/ewaters/perl-PDL-Graphics-ColorDistance
///
/// Computes the CIEDE2000 color-difference between two Lab colors
/// Based on the article:
/// The CIEDE2000 Color-Difference Formula: Implementation Notes,
/// Supplementary Test Data, and Mathematical Observations,", G. Sharma,
/// W. Wu, E. N. Dalal, submitted to Color Research and Application,
/// January 2004.
/// Available at http://www.ece.rochester.edu/~/gsharma/ciede2000/
/// Based on the C++ implementation by Ofir Pele, The Hebrew University of Jerusalem 2010.
//
static inline
void deltaE2000( double *lab1, double *lab2, double *delta_e )
{
	double Lstd = *lab1;
	double astd = *(lab1+1);
	double bstd = *(lab1+2);

	double Lsample = *lab2;
	double asample = *(lab2+1);
	double bsample = *(lab2+2);

//UNUSED	double _kL = 1.0;
//UNUSED	double _kC = 1.0;
//UNUSED	double _kH = 1.0;

	double Cabstd= sqrt(astd*astd+bstd*bstd);
	double Cabsample= sqrt(asample*asample+bsample*bsample);

	double Cabarithmean= (Cabstd + Cabsample)/2.0;

	double G= 0.5*( 1.0 - sqrt( pow(Cabarithmean,7.0)/(pow(Cabarithmean,7.0) + pow(25.0,7.0))));

	double apstd= (1.0+G)*astd; // aprime in paper
	double apsample= (1.0+G)*asample; // aprime in paper
	double Cpsample= sqrt(apsample*apsample+bsample*bsample);

	double Cpstd= sqrt(apstd*apstd+bstd*bstd);
	// Compute product of chromas
	double Cpprod= (Cpsample*Cpstd);


	// Ensure hue is between 0 and 2pi
	double hpstd= atan2(bstd,apstd);
	if (hpstd<0) hpstd+= 2.0*pi;  // rollover ones that come -ve

	double hpsample= atan2(bsample,apsample);
	if (hpsample<0) hpsample+= 2.0*pi;
	if ( (fabs(apsample)+fabs(bsample))==0.0)  hpsample= 0.0;

	double dL= (Lsample-Lstd);
	double dC= (Cpsample-Cpstd);

	// Computation of hue difference
	double dhp= (hpsample-hpstd);
	if (dhp>pi)  dhp-= 2.0*pi;
	if (dhp<-pi) dhp+= 2.0*pi;
	// set chroma difference to zero if the product of chromas is zero
	if (Cpprod == 0.0) dhp= 0.0;

	// Note that the defining equations actually need
	// signed Hue and chroma differences which is different
	// from prior color difference formulae

	double dH= 2.0*sqrt(Cpprod)*sin(dhp/2.0);
	//%dH2 = 4*Cpprod.*(sin(dhp/2)).^2;

	// weighting functions
	double Lp= (Lsample+Lstd)/2.0;
	double Cp= (Cpstd+Cpsample)/2.0;

	// Average Hue Computation
	// This is equivalent to that in the paper but simpler programmatically.
	// Note average hue is computed in radians and converted to degrees only
	// where needed
	double hp= (hpstd+hpsample)/2.0;
	// Identify positions for which abs hue diff exceeds 180 degrees
	if ( fabs(hpstd-hpsample)  > pi ) hp-= pi;
	// rollover ones that come -ve
	if (hp<0) hp+= 2.0*pi;

	// Check if one of the chroma values is zero, in which case set
	// mean hue to the sum which is equivalent to other value
	if (Cpprod==0.0) hp= hpsample+hpstd;

	double Lpm502= (Lp-50.0)*(Lp-50.0);;
	double Sl= 1.0+0.015*Lpm502/sqrt(20.0+Lpm502);
	double Sc= 1.0+0.045*Cp;
	double T= 1.0 - 0.17*cos(hp - pi/6.0) + 0.24*cos(2.0*hp) + 0.32*cos(3.0*hp+pi/30.0) - 0.20*cos(4.0*hp-63.0*pi/180.0);
	double Sh= 1.0 + 0.015*Cp*T;
	double delthetarad= (30.0*pi/180.0)*exp(- pow(( (180.0/pi*hp-275.0)/25.0),2.0));
	double Rc=  2.0*sqrt(pow(Cp,7.0)/(pow(Cp,7.0) + pow(25.0,7.0)));
	double RT= -sin(2.0*delthetarad)*Rc;

	// The CIE 00 color difference
	*delta_e = sqrt( pow((dL/Sl),2.0) + pow((dC/Sc),2.0) + pow((dH/Sh),2.0) + RT*(dC/Sc)*(dH/Sh) );
}

double
rgbCIEDE2000(	int std_r, int std_g, int std_b,
		int sam_r, int sam_g, int sam_b)
{
    struct labcolour standard, sample;
    struct xyzcolour xstandard, xsample;

    double deltaE = -1;
    double std_d[3];
    double sam_d[3];

    xstandard = sRGB_to_xyz(std_r, std_g, std_b);
    standard = xyz_to_lab(xstandard);

    xsample = sRGB_to_xyz(sam_r, sam_g, sam_b);
    sample = xyz_to_lab(xsample);

#if 0
    printf("\033[48;2;%d;%d;%dm std: \033[m #%02x%02x%02x -> %f,%f,%f -> %f,%f,%f\n",
	    std_r, std_g, std_b,
	    std_r, std_g, std_b,
	    xstandard.X, xstandard.Y, xstandard.Z,
	    standard.L, standard.a, standard.b);
    printf("\033[48;2;%d;%d;%dm sam: \033[m #%02x%02x%02x -> %f,%f,%f -> %f,%f,%f\n",
	    sam_r, sam_g, sam_b,
	    sam_r, sam_g, sam_b,
	    xsample.X, xsample.Y, xsample.Z,
	    sample.L, sample.a, sample.b);
#endif

    std_d[0] = standard.L;
    std_d[1] = standard.a;
    std_d[2] = standard.b;

    sam_d[0] = sample.L;
    sam_d[1] = sample.a;
    sam_d[2] = sample.b;

    deltaE2000(std_d, sam_d, &deltaE);

    return deltaE;
}

int
choose_XTterm256(int r, int g, int b)
{
    /* Annoyingly the 6x6x6 cube that XTerm uses by default (and so our cube)
     * isn't the websafe colours. This means the standard method of calculating
     * the best match won't work ... so I'll do a dumb search.
     */
    int best_diff = -1, nearest_static = 0;
    int c;
    for(c=16; c<256; c++) {
        int i = c-16, d, this_diff = 0;
        int nr, ng, nb;
        if (c<232) {
            nr = i / 36; ng = (i / 6) % 6; nb = i % 6;
            nr = nr ? nr * 40 + 55 : 0;
            ng = ng ? ng * 40 + 55 : 0;
            nb = nb ? nb * 40 + 55 : 0;
        } else {
            i = i - 216;
            nr=ng=nb = i * 10 + 8;
        }

	this_diff = 10000 * rgbCIEDE2000(r,g,b, nr,ng,nb);

        if (best_diff<0 || best_diff>this_diff) {
            nearest_static = c;
            best_diff = this_diff;
        }
    }

    return nearest_static;
}

