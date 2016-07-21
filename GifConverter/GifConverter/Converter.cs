using RGiesecke.DllExport;
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;

namespace GifConverter
{
    public class Converter
    {
        public delegate IntPtr RequestBinaryImageAlloc(int FrameIndex, long BinarySize);

        [DllExport("GetGifInfo", CallingConvention = CallingConvention.StdCall)]
        static public unsafe int GetGifInfo(IntPtr UE4GifBinaries, int UE4GifBinaryLength, RequestBinaryImageAlloc RequestAllocFunction)
        {
            byte[] GifBinaries = new byte[UE4GifBinaryLength];
            Marshal.Copy(UE4GifBinaries, GifBinaries, 0, UE4GifBinaryLength);
        
            using (Stream BinaryReader = new MemoryStream(GifBinaries))
            {
                Bitmap GifImage = (Bitmap)Image.FromStream(BinaryReader);
                Bitmap[] GifFrames = ParseFrames(GifImage);

                RequestBinaryImageAlloc RequestAlloc = new RequestBinaryImageAlloc(RequestAllocFunction);
                for (int frame = 0; frame < GifFrames.Length; frame++)
                {
                    Bitmap FrameImage = GifFrames[frame];
                    using (MemoryStream ImageBinary = new MemoryStream())
                    {
                        FrameImage.Save(ImageBinary, ImageFormat.Jpeg);
                        IntPtr AllocPtr = RequestAlloc(frame, ImageBinary.Length);

                        byte[] ImageBytes = ImageBinary.ToArray();
                        Marshal.Copy(ImageBytes, 0, AllocPtr, ImageBytes.Length);
                    }
                }
            }

            return 1;
        }

        public static Bitmap[] ParseFrames(Bitmap Animation)
        {
            int Length = Animation.GetFrameCount(FrameDimension.Time);

            Bitmap[] Frames = new Bitmap[Length];
            for (int Index = 0; Index < Length; Index++)
            {
                Animation.SelectActiveFrame(FrameDimension.Time, Index);
                Frames[Index] = new Bitmap(Animation.Size.Width, Animation.Size.Height);
                Graphics.FromImage(Frames[Index]).DrawImage(Animation, new Point(0, 0));
            }

            return Frames;
        }
    }
}
