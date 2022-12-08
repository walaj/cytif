#include <vips/vips8>
#include <iostream>
#include <string>

using vips::VImage;

int main(int argc, char **argv) {

  if (VIPS_INIT (argv[0])) 
    vips_error_exit (NULL);
  
  if (argc != 3)
    vips_error_exit ("usage: %s input-file output-file", argv[0]);
  
  std::cout << "THE INPUT IS: " << std::string(argv[1]) << std::endl;
    
  // Open the input image using the VImage class
  VImage image = VImage::new_from_file(argv[1],
				       VImage::option()->set("access", VIPS_ACCESS_SEQUENTIAL));

  // Compute the histogram of the image using the VImage::hist_find() method
  //VImage hist = image.hist_find();

  std::cout << " average " << image.avg() << std::endl;
  
  /*
  
  // Iterate over the histogram data and print the values to the standard output
  for (int i = 0; i < hist.width(); i++)
    {
      std::cout << hist(0,i) << std::endl;
    }
  */
  return 0;
}
