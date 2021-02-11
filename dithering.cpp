  [Description("Stucki")]
  public sealed class StuckiDithering : ErrorDiffusionDithering
  {
    #region Constructors

    public StuckiDithering()
      : base(new byte[,]
             {
               {
                 0, 0, 0, 8, 4
               },
               {
                 2, 4, 8, 4, 2
               },
               {
                 1, 2, 4, 2, 1
               }
             }, 42, false)
    { }

    #endregion
  }

  [Description("Atkinson")]
  public sealed class AtkinsonDithering : ErrorDiffusionDithering
  {
    #region Constructors

    public AtkinsonDithering()
      : base(new byte[,]
             {
               {
                 0, 0, 1, 1
               },
               {
                 1, 1, 1, 0
               },
               {
                 0, 1, 0, 0
               }
             }, 3, true)
    { }

    #endregion
  }

  public sealed class JarvisJudiceNinkeDithering : ErrorDiffusionDithering
  {
    #region Constructors

    public JarvisJudiceNinkeDithering()
      : base(new byte[,]
             {
               {
                 0, 0, 0, 7, 5
               },
               {
                 3, 5, 7, 5, 3
               },
               {
                 1, 3, 5, 3, 1
               }
             }, 48, false)
    { }

    #endregion
  }

    internal static byte ToByte(this int value)
    {
      if (value < 0)
      {
        value = 0;
      }
      else if (value > 255)
      {
        value = 255;
      }

      return (byte)value;
    }


D:\src\Dithering\src\Dithering\ErrorDiffusionDithering.cs
    bool IErrorDiffusion.Prescan
    { get { return false; } }

    void IErrorDiffusion.Diffuse(ArgbColor[] data, ArgbColor original, ArgbColor transformed, int x, int y, int width, int height)
    {
      int redError;
      int blueError;
      int greenError;

      redError = original.R - transformed.R;
      blueError = original.G - transformed.G;
      greenError = original.B - transformed.B;

      for (int row = 0; row < _matrixHeight; row++)
      {
        int offsetY;

        offsetY = y + row;

        for (int col = 0; col < _matrixWidth; col++)
        {
          int coefficient;
          int offsetX;

          coefficient = _matrix[row, col];
          offsetX = x + (col - _startingOffset);

          if (coefficient != 0 && offsetX > 0 && offsetX < width && offsetY > 0 && offsetY < height)
          {
            ArgbColor offsetPixel;
            int offsetIndex;
            int newR;
            int newG;
            int newB;
            byte r;
            byte g;
            byte b;

            offsetIndex = offsetY * width + offsetX;
            offsetPixel = data[offsetIndex];

            // if the UseShifting property is set, then bit shift the values by the specified
            // divisor as this is faster than integer division. Otherwise, use integer division

            if (_useShifting)
            {
              newR = (redError * coefficient) >> _divisor;
              newG = (greenError * coefficient) >> _divisor;
              newB = (blueError * coefficient) >> _divisor;
            }
            else
            {
              newR = redError * coefficient / _divisor;
              newG = greenError * coefficient / _divisor;
              newB = blueError * coefficient / _divisor;
            }

            r = (offsetPixel.R + newR).ToByte();
            g = (offsetPixel.G + newG).ToByte();
            b = (offsetPixel.B + newB).ToByte();

            data[offsetIndex] = ArgbColor.FromArgb(offsetPixel.A, r, g, b);
          }
        }
      }
    }

    #endregion
  }
}



    public ArgbColor Transform(ArgbColor[] data, ArgbColor pixel, int x, int y, int width, int height)
    {
      byte gray;

      gray = (byte)(0.299 * pixel.R + 0.587 * pixel.G + 0.114 * pixel.B);

      /*
       * I'm leaving the alpha channel untouched instead of making it fully opaque
       * otherwise the transparent areas become fully black, and I was getting annoyed
       * by this when testing images with large swathes of transparency!
       */

      return gray < _threshold ? _black : _white;
    }



    private void ProcessPixels(ArgbColor[] pixelData, Size size, IPixelTransform pixelTransform, IErrorDiffusion dither)
    {
      for (int row = 0; row < size.Height; row++)
      {
        for (int col = 0; col < size.Width; col++)
        {
          int index;
          ArgbColor current;
          ArgbColor transformed;

          index = row * size.Width + col;

          current = pixelData[index];

          // transform the pixel
          if (pixelTransform != null)
          {
            transformed = pixelTransform.Transform(pixelData, current, col, row, size.Width, size.Height);
            pixelData[index] = transformed;
          }
          else
          {
            transformed = current;
          }

          // apply a dither algorithm to this pixel
          // assuming it wasn't done before
          dither?.Diffuse(pixelData, current, transformed, col, row, size.Width, size.Height);
        }
      }
    }



    private Bitmap GetTransformedImage(WorkerData workerData)
    {
      Bitmap image;
      Bitmap result;
      ArgbColor[] pixelData;
      Size size;
      IPixelTransform transform;
      IErrorDiffusion dither;

      transform = workerData.Transform;
      dither = workerData.Dither;
      image = workerData.Image;
      size = image.Size;
      pixelData = image.GetPixelsFrom32BitArgbImage();

      if (dither != null && dither.Prescan)
      {
        // perform the dithering on the source data before
        // it is transformed
        this.ProcessPixels(pixelData, size, null, dither);
        dither = null;
      }

      // scan each pixel, apply a transform the pixel
      // and then dither it
      this.ProcessPixels(pixelData, size, transform, dither);

      // create the final bitmap
      result = pixelData.ToBitmap(size);

      return result;
    }
