#if HAVE_CONFIG_H
#include "config.h"
#endif
#if defined( __GNUG__ )
#pragma implementation
#endif

/*
 * equities.cc
 *
 * by Joseph Heled <joseph@gnubg.org>, 2000
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <stack>

#include "minmax.h"

#include "bgdefs.h"

#include "mec.h"

#include "equities.h"

using std::stack;
using std::cout;
using std::endl;

// Jacobs
// Table for equally strong players from Can a Fish Taste Twice as Good.

float
Equities::Jacobs[25][25] = {
{ .500, .690, .748, .822, .844, .894, .909, .938, .946, .963, .968, .978, .981,
  .987, .989, .992, .994, .996, .996, .997, .998, .998, .999, .999, .999, } ,

{ .310, .500, .603, .686, .753, .810, .851, .886, .910, .931, .946, .959, .967,
   .975, .980, .985, .988, .991, .993, .995, .996, .997, .998, .998, .999, } ,
  
{ .252, .397, .500, .579, .654, .719, .769, .814, .848, .879, .902, .922, .937,
  .950, .960, .969, .975, .981, .984, .988, .990, .993, .994, .995, .996, } ,
  
{ .178, .314, .421, .500, .579, .647, .704, .753, .795, .831, .861, .886, .907,
  .925, .939, .951, .960, .968, .974, .979, .983, .987, .989, .992, .993, } ,
  
{ .156, .247, .346, .421, .500, .568, .629, .683, .730, .773, .808, .840, .866,
  .889, .908, .924, .938, .949, .958, .966, .972, .978, .982, .985, .988, } ,
  
{ .106, .190, .281, .353, .432, .500, .563, .620, .672, .718, .759, .795, .826,
  .853, .876, .896, .913, .928, .940, .950, .959, .966, .972, .977, .981, } ,
  
{ .091, .149, .231, .296, .371, .437, .500, .557, .611, .660, .704, .744, .779,
  .811, .838, .862, .883, .901, .917, .930, .941, .951, .959, .966, .972, } ,
  
{ .062, .114, .186, .247, .317, .380, .443, .500, .555, .606, .653, .695, .734,
  .768, .799, .827, .851, .873, .891, .907, .921, .934, .944, .953, .961, } ,
  
{ .054, .090, .152, .205, .270, .328, .389, .445, .500, .551, .600, .644, .685,
  .723, .757, .788, .816, .840, .862, .881, .898, .913, .926, .937, .946, } ,
  
{ .037, .069, .121, .169, .227, .282, .340, .394, .449, .500, .549, .595, .638,
  .678, .715, .748, .779, .806, .831, .853, .872, .890, .905, .918, .930, } ,
  
{ .032, .054, .098, .139, .192, .241, .296, .347, .400, .451, .500, .546, .591,
  .632, .671, .706, .739, .769, .797, .821, .844, .864, .881, .897, .911, } ,
  
{ .022, .041, .078, .114, .160, .205, .256, .305, .356, .405, .454, .500, .545,
  .587, .627, .665, .700, .732, .761, .789, .813, .835, .856, .874, .890, } ,
  
{ .019, .033, .063, .093, .134, .174, .221, .266, .315, .362, .409, .455, .500,
  .543, .584, .622, .659, .693, .725, .754, .781, .805, .828, .848, .866, } ,
  
{ .013, .025, .050, .075, .111, .147, .189, .232, .277, .322, .368, .413, .457,
  .500, .541, .581, .618, .654, .687, .718, .747, .774, .798, .820, .841, } ,
  
{ .011, .020, .040, .061, .092, .124, .162, .201, .243, .285, .329, .373, .416,
  .459, .500, .540, .578, .614, .649, .682, .712, .741, .767, .791, .814, } ,
  
{ .008, .015, .031, .049, .076, .104, .138, .173, .212, .252, .294, .335, .378,
  .419, .460, .500, .539, .576, .611, .645, .677, .707, .735, .761, .785, } ,
  
{ .006, .012, .025, .040, .062, .087, .117, .149, .184, .221, .261, .300, .341,
  .382, .422, .461, .500, .537, .573, .608, .641, .672, .702, .729, .755, } ,
  
{ .004, .009, .019, .032, .051, .072, .099, .127, .160, .194, .231, .268, .307,
  .346, .386, .424, .463, .500, .536, .571, .605, .637, .668, .697, .724, } ,
  
{ .004, .007, .016, .026, .042, .060, .083, .109, .138, .169, .203, .239, .275,
  .313, .351, .389, .427, .464, .500, .535, .570, .602, .634, .664, .692, } ,
  
{ .003, .005, .012, .021, .034, .050, .070, .093, .119, .147, .179, .211, .246,
  .282, .318, .355, .392, .429, .465, .500, .534, .568, .600, .631, .660, } ,
  
{ .002, .004, .010, .017, .028, .041, .059, .079, .102, .128, .156, .187, .219,
  .253, .288, .323, .359, .395, .430, .466, .500, .534, .566, .598, .628, } ,
  
{ .002, .003, .007, .013, .022, .034, .049, .066, .087, .110, .136, .165, .195,
  .226, .259, .293, .328, .363, .398, .432, .466, .500, .533, .565, .596, } ,
  
{ .001, .002, .006, .011, .018, .028, .041, .056, .074, .095, .119, .144, .172,
  .202, .233, .265, .298, .332, .366, .400, .434, .467, .500, .532, .563, } ,
  
{ .001, .002, .005, .008, .015, .023, .034, .047, .063, .082, .103, .126, .152,
  .180, .209, .239, .271, .303, .336, .369, .402, .435, .468, .500, .531, } ,
  
{ .001, .001, .004, .007, .012, .019, .028, .039, .054, .070, .089, .110, .134,
  .159, .186, .215, .245, .276, .308, .340, .372, .404, .437, .469, .500, }
};

float
Equities::WoolseyHeinrich[15][15] =
{
  {0.50, 0.70, 0.75, 0.83, 0.85, 0.90, 0.91, 0.94, 0.95, 0.97, 0.97, 0.98,
   0.98, 0.99, 0.99},
  {0.30, 0.50, 0.60, 0.68, 0.75, 0.81, 0.85, 0.88, 0.91, 0.93, 0.94, 0.95,
   0.96, 0.97, 0.98},
  {0.25, 0.40, 0.50, 0.59, 0.66, 0.71, 0.76, 0.80, 0.84, 0.87, 0.90, 0.92,
   0.94, 0.95, 0.96},
  {0.17, 0.32, 0.41, 0.50, 0.58, 0.64, 0.70, 0.75, 0.79, 0.83, 0.86, 0.88,
   0.90, 0.92, 0.93},
  {0.15, 0.25, 0.34, 0.42, 0.50, 0.57, 0.63, 0.68, 0.73, 0.77, 0.81, 0.84,
   0.87, 0.89, 0.90},
  {0.10, 0.19, 0.29, 0.36, 0.43, 0.50, 0.56, 0.62, 0.67, 0.72, 0.76, 0.79,
   0.82, 0.85, 0.87},
  {0.09, 0.15, 0.24, 0.30, 0.37, 0.44, 0.50, 0.56, 0.61, 0.66, 0.70, 0.74,
   0.78, 0.81, 0.84},
  {0.06, 0.12, 0.20, 0.25, 0.32, 0.38, 0.44, 0.50, 0.55, 0.60, 0.65, 0.69,
   0.73, 0.77, 0.80},
  {0.05, 0.09, 0.16, 0.21, 0.27, 0.33, 0.39, 0.45, 0.50, 0.55, 0.60, 0.64,
   0.68, 0.72, 0.76},
  {0.03, 0.07, 0.13, 0.17, 0.23, 0.28, 0.34, 0.40, 0.45, 0.50, 0.55, 0.60,
   0.64, 0.68, 0.71},
  {0.03, 0.06, 0.10, 0.14, 0.19, 0.24, 0.30, 0.35, 0.40, 0.45, 0.50, 0.55,
   0.59, 0.63, 0.67},
  {0.02, 0.05, 0.08, 0.12, 0.16, 0.21, 0.26, 0.31, 0.36, 0.40, 0.45, 0.50,
   0.54, 0.58, 0.62},
  {0.02, 0.04, 0.06, 0.10, 0.13, 0.18, 0.22, 0.27, 0.32, 0.36, 0.41, 0.46,
   0.50, 0.54, 0.58},
  {0.01, 0.03, 0.05, 0.08, 0.11, 0.15, 0.19, 0.23, 0.28, 0.32, 0.37, 0.42,
   0.46, 0.50, 0.54},
  {0.01, 0.02, 0.04, 0.07, 0.10, 0.13, 0.16, 0.20, 0.24, 0.29, 0.33, 0.38,
   0.42, 0.46, 0.50}
};

//  Table generated by mec.c,
//  (Copyright (C) 1996  Claes Thornberg (claest@it.kth.se)), using 
//    Gammon rate 0.2600
//    Winning %   0.5000
//
//  This table is very close to the one used in Snowie 2.1, which is given
//  below for comparison.
//
float
Equities::mec26[25][25] =
{
{ 0.500, 0.685, 0.750, 0.818, 0.842, 0.892, 0.909, 0.936, 0.946, 0.962, 0.968, 0.978, 0.981, 0.987, 0.989, 0.992, 0.993, 0.995, 0.996, 0.997, 0.998, 0.998, 0.999, 0.999, 0.999, } ,
{ 0.315, 0.500, 0.595, 0.664, 0.737, 0.795, 0.835, 0.869, 0.896, 0.919, 0.935, 0.949, 0.959, 0.969, 0.975, 0.981, 0.984, 0.988, 0.990, 0.993, 0.994, 0.995, 0.996, 0.997, 0.998, } ,
{ 0.250, 0.405, 0.500, 0.571, 0.646, 0.710, 0.758, 0.800, 0.835, 0.867, 0.890, 0.911, 0.927, 0.942, 0.952, 0.962, 0.969, 0.975, 0.980, 0.984, 0.987, 0.990, 0.992, 0.993, 0.995, } ,
{ 0.182, 0.336, 0.429, 0.500, 0.575, 0.640, 0.694, 0.739, 0.781, 0.817, 0.847, 0.872, 0.894, 0.912, 0.927, 0.940, 0.950, 0.960, 0.967, 0.973, 0.978, 0.982, 0.985, 0.988, 0.990, } ,
{ 0.158, 0.263, 0.354, 0.425, 0.500, 0.567, 0.623, 0.673, 0.720, 0.761, 0.796, 0.827, 0.853, 0.876, 0.895, 0.912, 0.926, 0.939, 0.949, 0.958, 0.965, 0.971, 0.976, 0.980, 0.984, } ,
{ 0.108, 0.205, 0.290, 0.360, 0.433, 0.500, 0.559, 0.612, 0.662, 0.706, 0.746, 0.780, 0.811, 0.838, 0.861, 0.882, 0.900, 0.915, 0.928, 0.939, 0.949, 0.957, 0.964, 0.970, 0.975, } ,
{ 0.091, 0.165, 0.242, 0.306, 0.377, 0.441, 0.500, 0.553, 0.605, 0.652, 0.694, 0.732, 0.766, 0.797, 0.824, 0.848, 0.869, 0.888, 0.904, 0.918, 0.930, 0.940, 0.949, 0.957, 0.964, } ,
{ 0.064, 0.131, 0.200, 0.261, 0.327, 0.388, 0.447, 0.500, 0.552, 0.600, 0.645, 0.685, 0.722, 0.755, 0.785, 0.812, 0.836, 0.858, 0.877, 0.894, 0.908, 0.921, 0.932, 0.942, 0.951, } ,
{ 0.054, 0.104, 0.165, 0.219, 0.280, 0.338, 0.395, 0.448, 0.500, 0.549, 0.595, 0.637, 0.676, 0.712, 0.745, 0.774, 0.801, 0.825, 0.847, 0.867, 0.884, 0.899, 0.913, 0.924, 0.935, } ,
{ 0.038, 0.081, 0.133, 0.183, 0.239, 0.294, 0.348, 0.400, 0.451, 0.500, 0.547, 0.590, 0.631, 0.669, 0.703, 0.735, 0.765, 0.791, 0.816, 0.838, 0.857, 0.875, 0.891, 0.905, 0.917, } ,
{ 0.032, 0.065, 0.110, 0.153, 0.204, 0.254, 0.306, 0.355, 0.405, 0.453, 0.500, 0.544, 0.586, 0.625, 0.662, 0.695, 0.727, 0.756, 0.782, 0.807, 0.829, 0.848, 0.866, 0.883, 0.897, } ,
{ 0.022, 0.051, 0.089, 0.128, 0.173, 0.220, 0.268, 0.315, 0.363, 0.410, 0.456, 0.500, 0.542, 0.582, 0.620, 0.656, 0.689, 0.720, 0.748, 0.774, 0.798, 0.820, 0.840, 0.859, 0.875, } ,
{ 0.019, 0.041, 0.073, 0.106, 0.147, 0.189, 0.234, 0.278, 0.324, 0.369, 0.414, 0.458, 0.500, 0.540, 0.579, 0.616, 0.650, 0.682, 0.713, 0.741, 0.767, 0.791, 0.813, 0.833, 0.851, } ,
{ 0.013, 0.031, 0.058, 0.088, 0.124, 0.162, 0.203, 0.245, 0.288, 0.331, 0.375, 0.418, 0.460, 0.500, 0.539, 0.576, 0.612, 0.645, 0.677, 0.706, 0.734, 0.760, 0.784, 0.805, 0.826, } ,
{ 0.011, 0.025, 0.048, 0.073, 0.105, 0.139, 0.176, 0.215, 0.255, 0.297, 0.338, 0.380, 0.421, 0.461, 0.500, 0.538, 0.574, 0.608, 0.641, 0.672, 0.701, 0.728, 0.753, 0.777, 0.799, } ,
{ 0.008, 0.019, 0.038, 0.060, 0.088, 0.118, 0.152, 0.188, 0.226, 0.265, 0.305, 0.344, 0.384, 0.424, 0.462, 0.500, 0.536, 0.571, 0.605, 0.637, 0.667, 0.695, 0.722, 0.747, 0.771, } ,
{ 0.007, 0.016, 0.031, 0.050, 0.074, 0.100, 0.131, 0.164, 0.199, 0.235, 0.273, 0.311, 0.350, 0.388, 0.426, 0.464, 0.500, 0.535, 0.569, 0.602, 0.633, 0.663, 0.691, 0.717, 0.742, } ,
{ 0.005, 0.012, 0.025, 0.040, 0.061, 0.085, 0.112, 0.142, 0.175, 0.209, 0.244, 0.280, 0.318, 0.355, 0.392, 0.429, 0.465, 0.500, 0.534, 0.567, 0.599, 0.630, 0.659, 0.686, 0.712, } ,
{ 0.004, 0.010, 0.020, 0.033, 0.051, 0.072, 0.096, 0.123, 0.153, 0.184, 0.218, 0.252, 0.287, 0.323, 0.359, 0.395, 0.431, 0.466, 0.500, 0.533, 0.566, 0.597, 0.626, 0.655, 0.682, } ,
{ 0.003, 0.007, 0.016, 0.027, 0.042, 0.061, 0.082, 0.106, 0.133, 0.162, 0.193, 0.226, 0.259, 0.294, 0.328, 0.363, 0.398, 0.433, 0.467, 0.500, 0.532, 0.564, 0.594, 0.623, 0.651, } ,
{ 0.002, 0.006, 0.013, 0.022, 0.035, 0.051, 0.070, 0.092, 0.116, 0.143, 0.171, 0.202, 0.233, 0.266, 0.299, 0.333, 0.367, 0.401, 0.434, 0.468, 0.500, 0.532, 0.562, 0.592, 0.621, } ,
{ 0.002, 0.005, 0.010, 0.018, 0.029, 0.043, 0.060, 0.079, 0.101, 0.125, 0.152, 0.180, 0.209, 0.240, 0.272, 0.305, 0.337, 0.370, 0.403, 0.436, 0.468, 0.500, 0.531, 0.561, 0.590, } ,
{ 0.001, 0.004, 0.008, 0.015, 0.024, 0.036, 0.051, 0.068, 0.087, 0.109, 0.134, 0.160, 0.187, 0.216, 0.247, 0.278, 0.309, 0.341, 0.374, 0.406, 0.438, 0.469, 0.500, 0.530, 0.560, } ,
{ 0.001, 0.003, 0.007, 0.012, 0.020, 0.030, 0.043, 0.058, 0.076, 0.095, 0.117, 0.141, 0.167, 0.195, 0.223, 0.253, 0.283, 0.314, 0.345, 0.377, 0.408, 0.439, 0.470, 0.500, 0.530, } ,
{ 0.001, 0.002, 0.005, 0.010, 0.016, 0.025, 0.036, 0.049, 0.065, 0.083, 0.103, 0.125, 0.149, 0.174, 0.201, 0.229, 0.258, 0.288, 0.318, 0.349, 0.379, 0.410, 0.440, 0.470, 0.500, } ,

};

float
Equities::gnur[15][15] = {
{ 0.5000, 0.6818, 0.7526, 0.8138, 0.8470, 0.8890, 0.9104, 0.9345, 0.9466, 0.9609, 0.9681, 0.9768, 0.9811, 0.9863, 0.9887,  },
{ 0.3182, 0.5000, 0.6052, 0.6719, 0.7460, 0.8037, 0.8466, 0.8802, 0.9061, 0.9272, 0.9428, 0.9556, 0.9652, 0.9731, 0.9790,  },
{ 0.2474, 0.3948, 0.5000, 0.5745, 0.6469, 0.7121, 0.7648, 0.8059, 0.8415, 0.8721, 0.8966, 0.9172, 0.9333, 0.9467, 0.9569,  },
{ 0.1862, 0.3281, 0.4255, 0.5000, 0.5742, 0.6423, 0.6978, 0.7471, 0.7887, 0.8225, 0.8538, 0.8794, 0.9012, 0.9191, 0.9336,  },
{ 0.1530, 0.2540, 0.3531, 0.4258, 0.5000, 0.5675, 0.6273, 0.6795, 0.7245, 0.7669, 0.8042, 0.8341, 0.8606, 0.8837, 0.9029,  },
{ 0.1110, 0.1963, 0.2879, 0.3577, 0.4325, 0.5000, 0.5648, 0.6168, 0.6680, 0.7148, 0.7551, 0.7900, 0.8206, 0.8479, 0.8711,  },
{ 0.0896, 0.1534, 0.2352, 0.3022, 0.3727, 0.4352, 0.5000, 0.5546, 0.6081, 0.6592, 0.7047, 0.7420, 0.7766, 0.8077, 0.8345,  },
{ 0.0655, 0.1198, 0.1941, 0.2529, 0.3205, 0.3832, 0.4454, 0.5000, 0.5551, 0.6053, 0.6531, 0.6950, 0.7320, 0.7667, 0.7972,  },
{ 0.0534, 0.0939, 0.1585, 0.2113, 0.2755, 0.3320, 0.3919, 0.4449, 0.5000, 0.5517, 0.6010, 0.6448, 0.6861, 0.7234, 0.7574,  },
{ 0.0391, 0.0728, 0.1279, 0.1775, 0.2331, 0.2852, 0.3408, 0.3947, 0.4483, 0.5000, 0.5505, 0.5961, 0.6390, 0.6797, 0.7164,  },
{ 0.0319, 0.0572, 0.1034, 0.1462, 0.1958, 0.2449, 0.2953, 0.3469, 0.3990, 0.4495, 0.5000, 0.5473, 0.5915, 0.6331, 0.6730,  },
{ 0.0232, 0.0444, 0.0828, 0.1206, 0.1659, 0.2100, 0.2580, 0.3050, 0.3552, 0.4039, 0.4527, 0.5000, 0.5457, 0.5880, 0.6297,  },
{ 0.0189, 0.0348, 0.0667, 0.0988, 0.1394, 0.1794, 0.2234, 0.2680, 0.3139, 0.3610, 0.4085, 0.4543, 0.5000, 0.5428, 0.5853,  },
{ 0.0137, 0.0269, 0.0533, 0.0809, 0.1163, 0.1521, 0.1923, 0.2333, 0.2766, 0.3203, 0.3669, 0.4120, 0.4572, 0.5000, 0.5414,  },
{ 0.0113, 0.0210, 0.0431, 0.0664, 0.0971, 0.1289, 0.1655, 0.2028, 0.2426, 0.2836, 0.3270, 0.3703, 0.4147, 0.4586, 0.5000,  },
};

float
Equities::mec57[15][15] =
{
{ 57.0, 81.5, 81.5, 92.0, 92.0, 96.6, 96.6, 98.5, 98.5, 99.4, 99.4, 99.7, 99.7, 99.9, 99.9, } ,  
{ 32.5, 57.0, 70.1, 81.4, 86.7, 91.9, 94.2, 96.5, 97.5, 98.5, 98.9, 99.3, 99.5, 99.7, 99.8, } , 
{ 32.5, 48.0, 60.6, 72.1, 79.5, 86.1, 90.0, 93.3, 95.3, 96.9, 97.8, 98.6, 99.0, 99.4, 99.6, } , 
{ 18.5, 36.0, 50.1, 62.4, 71.6, 79.4, 84.8, 89.3, 92.3, 94.6, 96.2, 97.4, 98.2, 98.8, 99.2, } , 
{ 18.5, 29.6, 42.1, 53.9, 63.8, 72.4, 79.0, 84.5, 88.5, 91.7, 94.0, 95.7, 97.0, 97.9, 98.5, } , 
{ 10.6, 22.0, 34.0, 45.6, 55.9, 65.1, 72.7, 79.1, 84.1, 88.1, 91.2, 93.5, 95.3, 96.6, 97.6, } ,  
{ 10.6, 17.8, 28.1, 38.6, 48.7, 58.1, 66.2, 73.3, 79.1, 83.9, 87.8, 90.8, 93.1, 94.9, 96.3, } ,  
{  6.0, 13.3, 22.4, 32.1, 41.9, 51.3, 59.8, 67.3, 73.8, 79.3, 83.9, 87.6, 90.5, 92.8, 94.6, } ,  
{  6.0, 10.7, 18.3, 26.8, 35.9, 44.9, 53.4, 61.3, 68.3, 74.4, 79.6, 83.9, 87.4, 90.3, 92.6, } ,  
{  3.4,  7.9, 14.4, 22.0, 30.4, 39.0, 47.4, 55.4, 62.7, 69.2, 74.9, 79.8, 83.9, 87.4, 90.1, } ,  
{  3.4,  6.3, 11.6, 18.1, 25.6, 33.6, 41.7, 49.6, 57.1, 63.9, 70.1, 75.4, 80.1, 84.0, 87.3, } ,  
{  2.0,  4.7,  9.1, 14.7, 21.4, 28.7, 36.4, 44.1, 51.6, 58.6, 65.1, 70.9, 76.0, 80.4, 84.2, } ,  
{  2.0,  3.7,  7.2, 11.9, 17.8, 24.4, 31.6, 39.0, 46.3, 53.4, 60.0, 66.1, 71.6, 76.5, 80.7, } ,  
{  1.1,  2.8,  5.6,  9.6, 14.7, 20.6, 27.3, 34.2, 41.3, 48.3, 55.0, 61.3, 67.1, 72.3, 77.0, } ,  
{  1.1,  2.2,  4.5,  7.7, 12.1, 17.4, 23.4, 29.9, 36.6, 43.5, 50.2, 56.6, 62.5, 68.0, 73.0, } ,  
  };

//  Table that came with Snowie 2.1 presumably after the developers
//  had done simulations to determine good table.

float
Equities::Snowie[15][15] =
{
  {.500, .685, .748, .819, .843, .891, .907, .935, .944, .961, .967, .977,
   .980, .986, .988},
  {.315, .500, .594, .664, .735, .791, .832, .866, .893, .916, .932, .947,
   .957, .967, .973},
  {.252, .406, .500, .572, .645, .707, .755, .797, .832, .863, .887, .908,
   .924, .939, .950},
  {.181, .336, .428, .500, .573, .637, .690, .736, .777, .813, .843, .868,
   .890, .909, .924},
  {.157, .265, .355, .427, .500, .565, .621, .671, .716, .757, .792, .823,
   .849, .872, .892},
  {.109, .209, .293, .363, .435, .500, .558, .610, .659, .703, .742, .777,
   .807, .834, .857},
  {.093, .168, .245, .310, .379, .442, .500, .552, .603, .649, .691, .729,
   .762, .793, .820},
  {.065, .134, .203, .264, .329, .390, .448, .500, .551, .598, .642, .682,
   .718, .752, .781},
  {.056, .107, .168, .223, .284, .341, .397, .449, .500, .548, .593, .634,
   .673, .709, .741},
  {.039, .084, .137, .187, .243, .297, .351, .402, .452, .500, .546, .588,
   .628, .666, .700},
  {.033, .068, .113, .157, .208, .258, .309, .358, .407, .454, .500, .543,
   .584, .623, .659},
  {.023, .053, .092, .132, .177, .223, .271, .318, .366, .412, .457, .500,
   .542, .581, .618},
  {.020, .043, .076, .110, .151, .193, .238, .282, .327, .372, .416, .458,
   .500, .540, .578},
  {.014, .033, .061, .091, .128, .166, .207, .248, .291, .334, .377, .419,
   .460, .500, .538},
  {.012, .027, .050, .076, .108, .143, .180, .219, .259, .300, .341, .382,
   .422, .462, .500},
};

MatchState
Equities::match;

namespace {

// Set for money

float xCube2 = 1.0;
float xCube3 = 1.0;
float oCube2 = -1.0;
float oCube3 = -1.0;
}

extern "C" float
equity(float const p[NUM_OUTPUTS], bool const direction)
{
  return Equities::normalizedEquity(p, direction);
}


namespace Equities {

static float
postProbs[] =
{ 0.50000,
  0.484988779204,
  0.3195,
  0.302313595788,
  0.18935485,
  0.175375581897,
  0.11368904122,
  0.104665411717,
  0.067397551901,
  0.0615931228385,
  0.039882714542,
  0.0368401457886,
  0.023532346981,
  0.0220564138613,
  0.0143039149194,
  0.0128627481489,
  0.00849200040717,
  0.00770736867884,
  0.00509230940013,
  0.0046016493719,
  0.00305933550471,
  0.00272992874382,
  0.0018220307613,
  0.00164332944811
//    0.48715, 0.31400, 0.30270, 0.18600, 0.17715, 0.11200, 0.11200,
//    0.06600, 0.06600, 0.04000, 0.04000, 0.02400, 0.02400, 0.01400, 0.01400,
//    0.00800, 0.00800, 0.00400, 0.00400, 0.00200, 0.00200, 0.00200, 0.00200,
};

float
equitiesTable[25][25];

static stack<EqTable> eStack;
 
EqTable curEquities = 0;

void
push(const EqTable e)
{
  eStack.push(curEquities);
  curEquities = e;
}

const EqTable
pop(void)
{
  curEquities = eStack.top();
  eStack.pop();
  return curEquities;
}


Equities::EqTable
getTable(float const wpf, float const gr)
{
  float (*e)[25] = new float [25][25];

  uint const ml = 25;

  float* t[ml];
  for(uint k = 0; k < ml; ++k) {
    t[k] = e[k];
  }
  
  eqTable(ml, t, gr, wpf);

  return e;
}

void
set(float const wpf, float const gr)
{
  EqTable e = getTable(wpf, gr);
  
  set(e);
  
  delete[] e;
}

void
set(uint xAway, uint oAway, float val)
{
  if( oAway == 0 ) {
    if( xAway > 0 ) {
      postProbs[xAway - 1] = val;
    }
  } else {
    equitiesTable[xAway-1][oAway-1] = val;
  }
}

float
probPost(int const xAway)
{
  if( xAway <= 0 ) {
    return 1;
  }

  {         assert( uint(xAway) <= sizeof(postProbs)/sizeof(postProbs[0]) ); }
  
  return postProbs[xAway - 1];
  
//    if( xAway == 1 ) {
//      return 0.5;
//    }

//    if( xAway == 2 ) {
//      return (1.0 - 0.0257) / 2.0;
//    }

//    if( xAway == 4 ) {
//      return (1.0 - 0.3946) / 2.0;
//    }

//    if( xAway == 6 ) {
//      return (1.0 - 0.645702676537) / 2.0;
//    }

//    // BUG 8 and 9 comes out the same?
  
//    uint const o = xAway + ((xAway & 0x1) ? 2 : 1);
  
//    return 2 * prob(o, 1);
}

 
extern "C" float (*equityFunc)(float const p[NUM_OUTPUTS], bool direction);
 
// Compute Gammon/Backgammon cube values for currect score.

static void
recompute(void)
{
  equityFunc = ::equity;

  uint const xAway = match.xAway;
  uint const oAway = match.oAway;
  uint const cube = match.cube;
  
  if( xAway == 0 && oAway == 0 ) {
    xCube2 = 1.0;
    xCube3 = 1.0;
    oCube2 = -1.0;
    oCube3 = -1.0;
    return;
  }
  
  if( xAway == 1 && oAway == 1 ) {
    xCube2 = xCube3 = 0.0;
    oCube2 = oCube3 = 0.0;
    return;
  }

  if( xAway == 1 ) {
    float const p01 = 1 - probPost(oAway - cube);
    float const p02 = 1 - probPost(oAway - 2*cube);
    float const p03 = 1 - probPost(oAway - 3*cube);

    float const p10 = 1.0;
    
    float const center = (p10 + p01) / 2.0;
    float const one = p10 - center;
    
    float const cube2 = (p02 - center) / one;
    float const cube3 = (p03 - center) / one;

    oCube2 = cube2 + 1.0;
    oCube3 = cube3 - cube2;
    
    xCube2 = xCube3 = 0.0;

    return;
  }

  if( oAway == 1 ) {
    float const p10 = probPost(xAway - cube);
    float const p20 = probPost(xAway - 2*cube);
    float const p30 = probPost(xAway - 3*cube);

    float const p01 = 0.0;
    
    float const center = (p10 + p01) / 2.0;
    float const one = p10 - center;
    
    float const cube2 = (p20 - center) / one;
    float const cube3 = (p30 - center) / one;

    oCube2 = oCube3 = 0.0;

    xCube2 = cube2 - 1.0;
    xCube3 = cube3 - cube2;
    
    return;
  }

  
  float const p10 =  prob(xAway - cube, oAway);
  float const p01 =  prob(xAway, oAway - cube);
  float const p20 =  prob(xAway - 2*cube, oAway);
  float const p02 =  prob(xAway, oAway - 2*cube);
  float const p30 =  prob(xAway - 3*cube, oAway);
  float const p03 =  prob(xAway, oAway - 3*cube);
  
  float const center = (p10 + p01) / 2.0;
  float const one = p10 - center;
  
  float const x2 = (p20 - center) / one;
  float const o2 = (p02 - center) / one;

  float const x3 = (p30 - center) / one;
  float const o3 = (p03 - center) / one;

  xCube3 = x3 - x2;
  xCube2 = x2 - 1.0;

  oCube3 = o3 - o2;
  oCube2 = o2 + 1.0;
}

float
prob(int const xAway, int const oAway, bool const crawford)
{
  if( (xAway == 1 || oAway == 1) && ! crawford ) {
    if( xAway == 1 ) {
      return 1.0 - probPost(oAway);
    } else {
      return probPost(xAway);
    }
  }

  return prob(xAway, oAway);
}

static bool
setCube(uint const cube)
{
  if( cube == 0 || (cube & (cube-1)) != 0 ) {
    return false;
  }

  recompute();

  return true;
}

static bool
setMatchScore(uint const xAway, uint const oAway)
{
  if( ! (xAway <= 25 && oAway <= 25 ) ) {
    return false;
  }

  recompute();

  return true;
}


float
normalizedEquity(const float* const p, bool const xOnPlay)
{
  float e = 2 * p[WIN] - 1;
  
  if( xOnPlay ) {
    e += (xCube2 * p[WINGAMMON]  + xCube3 * p[WINBACKGAMMON] +
	  oCube2 * p[LOSEGAMMON] + oCube3 * p[LOSEBACKGAMMON]);
  } else {
    e -= (oCube2 * p[WINGAMMON]  + oCube3 * p[WINBACKGAMMON] +
	  xCube2 * p[LOSEGAMMON] + xCube3 * p[LOSEBACKGAMMON]);
  }

  return e;
}




static void
getPre(Es&          now,
       Es* const    doubled,
       uint const   xAway,
       uint const   oAway,
       uint const   cube,
       bool const   xOwns,
       float const  xGammonRatio,
       float const  oGammonRatio
#if defined( DEBUG )
       ,bool	const debug
#endif 
  )
{
#if defined( DEBUG )
  if( debug ) {
    cout << "get cube=" << cube << " -" << xAway << ",-" << oAway
	 << ((cube > 1) ? (xOwns ? " X owns " : " O owns ") : "")
	 << endl;
  }
#endif
  
  if( cube == 1 ) {
    // Equity at twice cube, after O doubles
    
    getPre(now, 0, xAway, oAway, 2*cube, true, xGammonRatio, oGammonRatio
#if defined( DEBUG )
	,debug
#endif
      );

    float const xdrop = value(xAway, oAway - cube);
    
    float const xl = now.x(xdrop);
    
    // Equity at twice cube, after X doubles
    
    getPre(now, 0, xAway, oAway, 2*cube, false, xGammonRatio, oGammonRatio
#if defined( DEBUG )
	,debug
#endif
	);
    
    float const odrop = value(xAway - cube, oAway);
    
    float const xh = now.x(odrop);

    if( doubled ) {
      *doubled = now;
    }
    
    now.xLow = xl;
    now.yLow = xdrop;
    
    now.xHigh = xh;
    now.yHigh = odrop;
    
  } else {
    if( xOwns ) {
      // X holds cube
      
      if( xAway <= cube ) {
	// cube dead, at 1 he wins

	now.xHigh = 1.0;
	now.yHigh = 1.0;

	if( doubled ) {
	  doubled->xHigh = 1.0;
	  doubled->yHigh = 1.0;
	  // if doubles (silly), opponent will re-double if needed
	  doubled->xLow = 0.0;
	  doubled->yLow = eqWhenLose(oGammonRatio, xAway, oAway, 4*cube);
	}
	
      } else {
	// Lets get X equity at twice cube with O holding.
      
	getPre(now, 0, xAway, oAway, 2*cube, !xOwns, xGammonRatio, oGammonRatio
#if defined( DEBUG )
	       ,debug
#endif
	    );

	// O will drop at this point
	
	float const edrop = value(xAway - cube, oAway);

	// which is rdp%  for X
	
	float const rdp = now.x(edrop);

	if( doubled ) {
	  *doubled = now;
	}
      
	now.yHigh = edrop;
	now.xHigh = rdp;
      }

      // at 0 he lose, xAway, oAway-cube or oAway-2*cube
      
      now.yLow = eqWhenLose(oGammonRatio, xAway, oAway, cube);
      now.xLow = 0;
      
    } else {
      if( oAway <= cube ) {
	// o holds cube, cube dead
	// at 0 o wins

	now.xLow = 0;
	now.yLow = -1;

	if( doubled ) {
	  doubled->xLow = 0;
	  doubled->yLow = -1;

	  // if doubles (silly), opponent will re-double if needed
	  doubled->xHigh = 1.0;
	  doubled->yHigh = eqWhenWin(xGammonRatio, xAway, oAway, 4*cube);
	}

      } else {

	getPre(now, 0, xAway, oAway, 2*cube, !xOwns, xGammonRatio, oGammonRatio
#if defined( DEBUG )
	       ,debug
#endif
	    );
	
	float const edrop = value(xAway, oAway - cube);

	float rdp = now.x(edrop);

	if( doubled ) {
	  *doubled = now;
	}
	
	now.xLow = rdp;
	now.yLow = edrop;
      }

      // to win X needs to climb to 100% (1)
      
      now.xHigh = 1.0;
      now.yHigh = eqWhenWin(xGammonRatio, xAway, oAway, cube);
    }
  }

#if defined( DEBUG )
  if( debug ) {
    cout << "get cube=" << cube << " -" << xAway << ",-" << oAway
	 << ((cube > 1) ? (xOwns ? " X owns " : " O owns ") : "")
	 << endl;
    
    cout << " return now = " << now;
    if( doubled ) {
      cout << " doubled " << *doubled;
    }
    cout << endl;
  }
#endif

}

void
get(Es&          now,
    Es* const    doubled,
    uint const   xAway,
    uint const   oAway,
    uint const   cube,
    float const  xGammonRatio,
    float const  oGammonRatio
#if defined( DEBUG )
    ,int	const debug
#endif 
  )
{
#if defined( DEBUG )
    if( debug == 1 ) {
      cout << "get cube=" << cube << " -" << xAway << ",-" << oAway
	   << ((cube > 1) ? " X owns " : "") << endl;
    }
#endif

  if( xAway > 1 && oAway > 1 ) {
    getPre(now, doubled, xAway, oAway, cube, true, xGammonRatio, oGammonRatio
#if defined( DEBUG )
	   ,debug > 1
#endif
	 );
  } else {
    // post crawford
    
    if( xAway == 1 && oAway == 1 ) {
      now.xHigh = 1.0;
      now.yHigh = 1.0;
      now.xLow = 0.0;
      now.yLow = -1.0;

      if( doubled ) {
	*doubled = now;
      }
    
      return;
    }

    if( cube == 1 ) {
      int const away = xAway == 1 ? oAway : xAway;
    
      float const gr = xAway == 1 ? oGammonRatio : xGammonRatio;
    
      now.xLow = 0.0;
      now.yLow = -1.0;
      
      now.xHigh = 1.0;
      now.yHigh = eWhenWinPost(gr, away, 2*cube);

      float const odrop = ePost(away - cube);
      float const xh = now.x(odrop);

      if( doubled ) {
	*doubled = now;
      }

      now.xHigh = xh;
      now.yHigh = odrop;

      if( xAway == 1 ) {
	now.reverse();
	if( doubled ) {
	  doubled->reverse();
	}
      }
    } else {
      // post crawford, cube turned
      
      if( oAway == 1 ) {
	// 1-away doubled. is it you kaufmann?
	
	now.xLow = 0.0;
	now.yLow = -1.0;
	
	now.xHigh = 1.0;
	now.yHigh = eWhenWinPost(xGammonRatio, xAway, 2*cube);
	
	if( doubled ) {
	  *doubled = now;
	}
      } else {
	now.xHigh = 1.0;
	now.yHigh = 1.0;
      
	now.xLow = 0.0;
	now.yLow = -eWhenWinPost(oGammonRatio, oAway, cube);

	if( doubled ) {
	  *doubled = now;
	
	  doubled->yLow = -eWhenWinPost(oGammonRatio, oAway, 4*cube);
	}
      }
    }
  }
  
#if defined( DEBUG )
  if( debug == 1 ) {
    cout << " return now = " << now;
    if( doubled ) {
      cout << " doubled " << *doubled;
    }
    cout << endl;
  }
#endif
}

void
getCrawfordEq(Es&          now,
	      uint const   away,
	      float const  gammonRatio)
{
  now.xLow = 0.0;
  now.yLow = -1.0;
  
  now.xHigh = 1.0;
  
  now.yHigh = eWhenWinPost(gammonRatio, away, 1);
}

void
getCrawfordEq(Es&          e,
	      uint const   xAway,
	      uint const   oAway,
	      float const  xgr,
	      float const  ogr)
{
  if( xAway == 1 ) {
    getCrawfordEq(e, oAway, ogr);
    e.reverse();
  } else {
    getCrawfordEq(e, xAway, xgr);
  }
}

void
get(Es&           now,
    Es*           doubled,
    uint const    xAway,
    uint const    oAway,
    uint const    cube,
    float const   xGammonRatio,
    float const   oGammonRatio,
    bool const    crawfordGame
#if defined( DEBUG )
    ,int const    debug
#endif
  )
{
  if( ! crawfordGame ) {
    get(now, doubled, xAway, oAway, cube, xGammonRatio, oGammonRatio
#if defined( DEBUG )
	, debug
#endif
	);
  } else {
    {                                     assert( xAway == 1 || oAway == 1 ); }
    
    getCrawfordEq(now, xAway, oAway, xGammonRatio, oGammonRatio);
    
    if( doubled ) {
      // ??? nonsense, no doubled cube in crawford
      *doubled = now;
    }
  }
}



class ApproxData {
public:
  ApproxData(void);

  void	get(Es&    now,
	    Es&    doubled,
	    float  xGammonRatio,
	    float  oGammonRatio) const;

  void	set(uint  xAway,
	    uint  oAway,
	    uint  cube);
  
  uint 		xAway;
  uint 		oAway;
  uint 		cube;

  Es		cur[21][21];
  Es		dou[21][21];
};

inline
ApproxData::ApproxData(void) :
  xAway(0),
  oAway(0),
  cube(0)
{}

void
ApproxData::set(uint const  xAway_,
		uint const  oAway_,
		uint const  cube_)
{
  xAway = xAway_;
  oAway = oAway_;
  cube = cube_;
  
  for(uint ix = 0; ix <= 20; ++ix) {
    for(uint iy = 0; iy <= 20; ++iy) {
      Equities::get(cur[ix][iy], &dou[ix][iy], xAway, oAway, cube, /*true,*/
		    float(ix) / 20.0, float(iy) / 20.0
#if defined( DEBUG )
		    ,false 
#endif
		    );
    }
  }
}

static inline float
interpolate(float const  ax,
	    float const  ay,
	    float const  c00,
	    float const  c01,
	    float const  c10,
	    float const  c11)
{
  float const x0010 = (1-ax) * c00 + ax * c10;
  float const x0111 = (1-ax) * c01 + ax * c11;

  return (1-ay) * x0010 + ay * x0111;
}

void
ApproxData::get(Es&          now,
		Es&          doubled,
		float const  xGammonRatio,
		float const  oGammonRatio) const 
{
  float const x = 20 * xGammonRatio;
  float const y = 20 * oGammonRatio;
  
  uint const ix = uint(x);
  uint const iy = uint(y);

  float const ax = x - ix;
  float const ay = y - iy;

  {
    Es const& c00 = cur[ix][iy];
    Es const& c01 = cur[ix+1][iy];
    Es const& c10 = cur[ix][iy+1];
    Es const& c11 = cur[ix+1][iy+1];
  
    now.xLow =
      interpolate(ax, ay, c00.xLow, c01.xLow, c10.xLow, c11.xLow);
    now.xHigh =
      interpolate(ax, ay, c00.xHigh, c01.xHigh, c10.xHigh, c11.xHigh);
    now.yLow =
      interpolate(ax, ay, c00.yLow, c01.yLow, c10.yLow, c11.yLow);
    now.yHigh =
      interpolate(ax, ay, c00.yHigh, c01.yHigh, c10.yHigh, c11.yHigh);
  }

  {
    Es const& c00 = dou[ix][iy];
    Es const& c01 = dou[ix+1][iy];
    Es const& c10 = dou[ix][iy+1];
    Es const& c11 = dou[ix+1][iy+1];
  
    doubled.xLow =
      interpolate(ax, ay, c00.xLow, c01.xLow, c10.xLow, c11.xLow);
    doubled.xHigh =
      interpolate(ax, ay, c00.xHigh, c01.xHigh, c10.xHigh, c11.xHigh);
    doubled.yLow =
      interpolate(ax, ay, c00.yLow, c01.yLow, c10.yLow, c11.yLow);
    doubled.yHigh =
      interpolate(ax, ay, c00.yHigh, c01.yHigh, c10.yHigh, c11.yHigh);
  }
}

static ApproxData
ap1, ap2;



void
getApproximat(Es&          now,
	      Es&          doubled,
	      uint const   xAway,
	      uint const   oAway,
	      uint const   cube,
	      float const  xGammonRatio,
	      float const  oGammonRatio
#if defined( DEBUG )
	      ,bool  const debug
#endif 
  )
{
  const ApproxData* ap = 0;
  
  if( xAway == ap1.xAway && oAway == ap1.oAway && cube == ap1.cube ) {
    ap = &ap1;
  } else if( xAway == ap2.xAway && oAway == ap2.oAway && cube == ap2.cube ) {
    ap = &ap2;
  } else if( xAway == ap1.oAway && oAway == ap1.xAway && cube == ap1.cube ) {
    ap2.set(xAway, oAway, cube);
    
    ap = &ap2;
  } else {
    ap1.set(xAway, oAway, cube);
    
    ap = &ap1;
  }

  ap->get(now, doubled, xGammonRatio, oGammonRatio);

#if defined( DEBUG )
  if( debug ) {
    cout << "get cube=" << cube << " -" << xAway << ",-" << oAway
	 << ((cube > 1) ? " X owns " : "")
	 << endl;
    
    cout << " return now = " << now;
    cout << " doubled " << doubled;
    cout << endl;
  }
#endif
  
}

static float
mwc0(const float* const  ar,
     bool const          xOnPlay,
     MatchState const&   match)
{
  float const xWins = xOnPlay ? ar[WIN] : 1 - ar[WIN];
  float const xGammons = xOnPlay ? ar[WINGAMMON] : ar[LOSEGAMMON];
  float const oWins = 1 - xWins;
  float const oGammons = xOnPlay? ar[LOSEGAMMON] : ar[WINGAMMON];

  float const xGammonRatio = (xWins > 0) ? xGammons / xWins : 0.0;
  float const oGammonRatio = (oWins > 0) ? oGammons / oWins : 0.0;

  // perfect live cube window
  Equities::Es el;

  bool const crawfordGame = match.crawfordGame;
  uint const xAway = match.xAway;
  uint const oAway = match.oAway;
  uint const cube = match.cube;
  
  if( cube == 1 || match.xOwns || crawfordGame ) {
    Equities::get(el, 0, xAway, oAway, cube, xGammonRatio, oGammonRatio,
		  crawfordGame
#if defined( DEBUG )
		  /* ,Analyze::adebug >= 1 */ ,false
#endif
      );
      
  } else {
    // O owns a > 1 cube, non crawford
    
    Equities::get(el, 0, oAway, xAway, cube, oGammonRatio, xGammonRatio,
		  false
#if defined( DEBUG )
		  /* ,Analyze::adebug >= 1 */ ,false
#endif
      );
      el.reverse();
  }

  uint const eCube =
    (cube == 1 && (xAway == 1 || oAway == 1) && ! crawfordGame) ? 2 : cube;
  
  float const xBGammons = xOnPlay ? ar[WINBACKGAMMON] : ar[LOSEBACKGAMMON];
  float const xbgr = xGammons > 0 ? (xBGammons / xGammons) : 0.0;

  float const oBGammons = xOnPlay ? ar[LOSEBACKGAMMON] : ar[WINBACKGAMMON];
  float const obgr = oGammons > 0 ? (oBGammons / oGammons) : 0.0;

  {
    float const hdif = oAway > 1 ?
      (Equities::value(xAway - 3*eCube, oAway) -
       Equities::value(xAway - 2*eCube, oAway)) :
      (Equities::ePost(xAway - 3*eCube) - (Equities::ePost(xAway - 2*eCube)));
    
    el.yHigh += xbgr * xGammonRatio * hdif;

    if( el.yHigh > 1.0 ) el.yHigh = 1;

    float const ldif = xAway > 1 ?
      (Equities::value(xAway, oAway - 3*eCube) -
       Equities::value(xAway, oAway - 2*eCube)) :
      
      -(Equities::ePost(oAway - 3*eCube) - (Equities::ePost(oAway - 2*eCube)));
      
    el.yLow += obgr * oGammonRatio * ldif;
    
    if( el.yLow < -1.0 ) el.yLow = -1.0;
  }

  Equities::Es dead = el;
  
  if( xWins > el.xHigh ) {
    el.xLow = el.xHigh;
    el.yLow = el.yHigh;

    el.xHigh = 1.0;
    el.yHigh = Equities::eWhenWin(xGammonRatio, xbgr, xAway, oAway, eCube);
  } else if( xWins < el.xLow ) {
    
    el.xHigh = el.xLow;
    el.yHigh = el.yLow;

    el.xLow = 0;
    el.yLow = Equities::eWhenLose(oGammonRatio, obgr, xAway, oAway, eCube);
  }


  float const elv = el.y(xWins);

  float ed;

  if( (xAway == 1 || oAway == 1) ) {
    ed = elv;
  } else {
    
    float const dyh =
      el.xHigh == 1.0 ? el.yHigh :
      Equities::eWhenWin(xGammonRatio, xbgr, xAway, oAway, cube);
    
    float const dyl =
      el.xLow == 0.0 ? el.yLow :
      Equities::eWhenLose(oGammonRatio, obgr, xAway, oAway, cube);
      
    if( cube == 1 ) {
      if( xWins > dead.xHigh ) {
	dead.xHigh = 1.0;  dead.yHigh = dyh;

	ed = (dead.y(xWins) + elv) / 2;

      } else if( xWins < dead.xLow ) {
	dead.xLow = 0.0;  dead.yLow = dyl;

	ed = (dead.y(xWins) + elv) / 2;

      } else {

	dead.xLow = 0; dead.yLow = dyl;
	ed = dead.y(xWins);

	dead = el;
	dead.xHigh = 1.0; dead.yHigh = dyh;
	ed += dead.y(xWins);

	ed /= 2.0;
      }
    } else {
      if( match.xOwns ) {
	dead.xHigh = 1.0;
	dead.yHigh = dyh;
      } else {
	dead.xLow = 0;
	dead.yLow = dyl;
      }
    
      ed = dead.y(xWins);
    }
  }

  float const cubeLife = 0.78;
  
  float const e1 = cubeLife * elv + (1-cubeLife) * ed;
  
  return xOnPlay ? e1 : -e1;
}



float
mwc(const float* const p, bool const xOnPlay)
{
  if( match.xAway == 0 && match.oAway == 0 ) {
    // money
    return normalizedEquity(p, xOnPlay);
  }
  
  return mwc0(p, xOnPlay, match);
}

}

void
MatchState::reset(void)
{
  xAway = 0;
  oAway = 0;
  cube = 1;
  xOwns = false;
  crawfordGame = false;

  Equities::recompute();
}

bool
MatchState::set(uint        xAway_,
		uint        oAway_,
		uint        cube_,
		bool const  xOwns_,
		int         crawford)
{
  if( xAway_ == 0 ) {
    xAway_ = xAway;
  }

  if( oAway_ == 0 ) {
    oAway_ = oAway;
  }

  bool ok = true;
  
  if( xAway != xAway_ || oAway_ != oAway ) {
    xAway = xAway_;
    oAway = oAway_;
    ok = Equities::setMatchScore(xAway, oAway);
  }

  if( cube_ != 0 && cube_ != cube ) {
    cube = cube_;
    xOwns = xOwns_;

    ok = Equities::setCube(cube) && ok;
  }
  
  if( crawford >= 0 ) {
    crawfordGame = crawford;
    
    assert( !crawford ||
	    (xAway == 1 && oAway > 1) || (oAway == 1 && xAway > 1) );
  }

  return ok;
}

float
MatchState::range(void) const
{
  Equities::Es e;
  if( cube > 1 && ! xOwns ) {
    Equities::get(e, 0, oAway, xAway, cube, 0.26, 0.26,
		  crawfordGame
#if defined( DEBUG )
		  ,false 		
#endif
      );
    e.reverse();
  } else {
    Equities::get(e, 0, xAway, oAway, cube, 0.26, 0.26,
		  crawfordGame
#if defined( DEBUG )
		  ,false 		
#endif
      );
  }
  
  return (e.yHigh - e.yLow)/(e.xHigh - e.xLow);
}

#if defined( DEBUG )
std::ostream&
operator <<(std::ostream& s, Equities::Es const& e)
{
  s << "(<" << e.xLow << "," << e.yLow << "> <"
    << e.xHigh << "," << e.yHigh << ">)";

  return s;
}
#endif

