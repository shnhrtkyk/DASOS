#include "HyperMap.h"
#include "Grid.h"
#include "binfile.h"

//-----------------------------------------------------------------------------
HyperMap::HyperMap(
        const std::string &i_name,
        Volume *i_obj,
        const unsigned short int i_band,
        const std::string &i_bilFileName,
        const std::string &i_IGMfileName
        ):
    Map(i_name,i_obj),
    m_IGMfileName(i_IGMfileName),
    m_hyperData(NULL)
{
   // read the band of our interest and put the data into an array
   try
   {
      bilLib::BinFile file(i_bilFileName);
      unsigned int nsamps=bilLib::StringToUINT(file.FromHeader("samples"));
      unsigned int nlines=bilLib::StringToUINT(file.FromHeader("lines"));
      unsigned int nbands=bilLib::StringToUINT(file.FromHeader("bands"));

      if(i_band>nbands)
      {
          std::cout<<"The selected Band does not exist!\n";
          return;
      }

      m_hyperData = new unsigned short int[nlines*nsamps];
      file.Readband((char *)m_hyperData,i_band);

      unsigned int k= 0;
      while(m_hyperData[k]==0 && k<nsamps*nlines)
      {
         k++;
      }
      unsigned int short min = m_hyperData[k];
      unsigned int short max = m_hyperData[k];
      for(unsigned int i=k; i<nsamps*nlines; ++i)
      {
         if(min>m_hyperData[i] && m_hyperData[i]!=0)
         {
            min = m_hyperData[i];
         }
         else if (max<m_hyperData[i])
         {
            max = m_hyperData[i];
         }
      }
      for(unsigned int i=0; i<nsamps*nlines; ++i)
      {
         if(m_hyperData[i]!=0)
         {
             m_hyperData[i] = double(double(m_hyperData[i]-min)/(max-min))*255;
         }
      }
      file.Close();
   }
   catch(bilLib::BinaryReader::BRexception e)
   {
      std::cout<<e.what()<<std::endl;
      std::cout<<e.info<<std::endl;
   }
}


//-----------------------------------------------------------------------------
void HyperMap::createMap()
{
   Grid *grid = new Grid(m_IGMfileName,30);
   const gmtl::Vec3f &mins = m_object->getMinLimits();
   const float vl(m_object->getVoxelLen());

   std::cout << vl << "\n -----------------------------------\n";

   const unsigned int noX = m_object->getNoVoxelsX();
   const unsigned int noY = m_object->getNoVoxelsY();
   for(unsigned int x=0; x<noX; ++x)
   {
      for(unsigned int y=0; y<noY; ++y)
      {
         const gmtl::Vec2f point(mins[0]+vl/2+ vl*x,mins[1]+vl/2+vl*y) ;
         unsigned int index = grid->getClosestPixelPosition(point[0],point[1]);
         m_mapValues[x+y*m_noOfPixelsX]= m_hyperData[index];
      }
   }
   delete grid;
}

//-----------------------------------------------------------------------------
HyperMap::~HyperMap()
{
   if(m_hyperData!=NULL)
   {
      delete m_hyperData;
   }
}
