
#include "image.hpp"


// > class Image

/*****************************************
* > Image()                              *
* Constructs and image from a file path. *
*****************************************/
Image::Image(const string& imgPath):
		imgPath(imgPath), img(imgPath.c_str()),
		width(img.width()), height(img.height()), channels(img.spectrum()) {
	
	if (channels != 1 && channels != 3) {
		throw std::runtime_error(
				"Image(). Wrong image format: " + std::to_string(channels) +
				" channels; only RGB and Grayscale supported");
	}
}


/*******************************************************
* > Image()                                            *
* Constructs a new image with the given size.          *
*                                                      *
* Args:                                                *
*   width (size_t), height (size_t), channels (size_t) *
*******************************************************/
Image::Image(size_t width, size_t height, size_t channels):
		imgPath("new_img.png"), img(width, height, 1, channels),
		width(width), height(height), channels(channels) {

	if (channels != 1 && channels != 3) {
		throw std::invalid_argument(
				"Image(). Wrong image format: " + std::to_string(channels) +
				" channels; only RGB and Grayscale supported");
	}
}


/*******************************************************
* > Image()                                            *
* Constructs a new image with the given size.          *
*                                                      *
* Args:                                                *
*   width (size_t), height (size_t), channels (size_t) *
*   val (double): initial value of each pixel          *
*******************************************************/
Image::Image(size_t width, size_t height, size_t channels, double val):
		imgPath("new_img.png"), img(width, height, 1, channels, val),
		width(width), height(height), channels(channels) {

	if (channels != 1 && channels != 3) {
		throw std::invalid_argument(
				"Image(). Wrong image format: " + std::to_string(channels) +
				" channels; only RGB and Grayscale supported");
	}
}


/********************************************************************
* > Image()                                                         *
* Constructs a new image moving the CImg into the current instance. *
*                                                                   *
* Args:                                                             *
*   im (CImg<double>&&): image to be moved                          *
********************************************************************/
Image::Image(CImg<double>&& im):
		imgPath("new_img.png"),
		width(im.width()),
		height(im.height()),
		channels(im.spectrum()) {
	im.move_to(img);
}


/***************************************************************************
* > size()                                                                 *
* Args:                                                                    *
*   dim (unsigned): dimension number. { 0: width, 1: height, 2: channels } *
*                                                                          *
* Returns:                                                                 *
*   (size_t): lenght of dimension 'dim'                                    *
***************************************************************************/
size_t Image::size(unsigned dim) const {
	switch (dim) {
		case 0:
			return width;
		case 1:
			return height;
		case 2:
			return channels;
		default:
			throw std::domain_error(string() +
					"size(). " + std::to_string(dim) +
					" is not a dimension {0,1,2}");
	}
}


/**********************************************************************
* > at()                                                              *
* Returns the intensity of the c channel. The result is obtained with *
* bilinear interpolation in the x,y directions.                       *
* For fast, discrete lookup, use the () operator.                     *
*                                                                     *
* Args:                                                               *
*   w (double): a continuous coordinate in the x direction            *
*   h (double): a continuous coordinate in the y direction            *
*   c (size_t): the channel to read                                   *
*                                                                     *
* Return:                                                             *
*   (double): the interpolated intensity                              *
**********************************************************************/
double Image::at(double w, double h, size_t c) const {

	// checks
	if (c >= channels) {
		throw std::domain_error(string() +
				"at(). " + "Wrong channel, " + std::to_string(c) +
				" of [0, " + std::to_string(channels-1) + "]");
	}
	if (w > (width-1) || h > (height-1) || w < 0 || h < 0) {
		throw std::domain_error(string() +
				"at(). " + "Wrong coorinate, (" + std::to_string(w) +
				", " + std::to_string(h) + ") of (0, 0) -- (" +
				std::to_string(width-1) + ", " + std::to_string(height-1) + ")");
	}

	// Get extremes
	size_t wLow = std::floor(w);
	size_t hLow = std::floor(h);
	size_t wHigh = std::ceil(w);
	size_t hHigh = std::ceil(h);
	double x = w - wLow;
	double y = h - hLow;

	double z00 = img(wLow, hLow, c);
	double z01 = img(wLow, hHigh, c);
	double z10 = img(wHigh, hLow, c);
	double z11 = img(wHigh, hHigh, c);

	// linear
	double z = z00 * (1 - x) * (1 - y) + 
			z10 * x * (1 - y) + 
			z01 * (1 - x) * y + 
			z11 * x * y;
	return z;
}


/**********************************************************************
* > atH()                                                             *
* Returns the intensity of the c channel. The result is obtained with *
* linear interpolation in the horizontal direction.                   *
* NOTE: bounds are not checked.                                       *
* For fast, discrete lookup, use the () operator.                     *
* For bilinear interpolation use at().                                *
*                                                                     *
* Args:                                                               *
*   w (double): a continuous coordinate in the x direction            *
*   h (size_t): the line to read                                      *
*   c (size_t): the channel to read                                   *
*                                                                     *
* Return:                                                             *
*   (double): the interpolated intensity                              *
**********************************************************************/
double Image::atH(double w, size_t h, size_t c) const {

	// Get extremes
	size_t wLow = std::floor(w);
	size_t wHigh = std::ceil(w);
	double x = w - wLow;

	double z0 = img(wLow, h, c);
	double z1 = img(wHigh, h, c);

	// linear
	double z = z0 * (1 - x) + z1 * x;

	return z;
}


/*****************************************************************************
* > toGrayscale()                                                            *
* Creates a new grayscale image from the current one using the ``luminance'' *
* formula.                                                                   *
*                                                                            *
* Returns:                                                                   *
*   (Image): a new image with the same width and height.                     *
*****************************************************************************/
Image Image::toGrayscale(void) const {

	// If already gray return a new image with the same properties
	if (channels == 1) {
		Image copyImg(*this);
		return copyImg;
	}

	// Create the new image (uninitialized)
	Image grayscale(width, height, 1);
	
	// Compute the gray pixels
	for (size_t w = 0; w < width; ++w) {
		for (size_t h = 0; h < height; ++h) {
			double red = img(w, h, 0, 0);
			double green = img(w, h, 0, 1);
			double blue = img(w, h, 0, 2);

			double gray = red * 0.3 + green * 0.59 + blue * 0.11;
			grayscale.img(w, h) = gray;
		}
	}

	return grayscale;
}


/***************************************************************
* > operator=                                                  *
* Assingn a CImg to the current instance with a move operation *
*                                                              *
* Args:                                                        *
*   im (CImg<double>&&): image to be moved                     *
*                                                              *
* Returns:                                                     *
*   (Image&): reference to this                                *
***************************************************************/
Image& Image::operator=(CImg<double>&& im) {

	imgPath = "new_img.png";
	width = im.width();
	height = im.height();
	channels = im.spectrum();
	im.move_to(img);

	return *this;
}


// print
std::ostream& operator<<(std::ostream& out, const Image& img) {
	return out << "Image: " << img.imgPath << ", size: (" << img.width <<
		"," << img.height << "," << img.channels << ")";
}
