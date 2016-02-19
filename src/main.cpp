#include <iostream>
#include "binfile.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <math.h>
#include "MapsManager.h"
#include "Las1_3_handler.h"
#include "MarchingCubes.h"
#include <exception>
#include <chrono>
#include <thread>

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>


#include "VolumeFactory.h"

#include "PW_handler.h"
#include <unordered_map>


#include "binfile.h"
#include  "commonfunctions.h"
#include "DtmBilReader.h"


int main (int argc, char const* argv[])
{
   // PARAMETERS
   std::vector<std::string> lasFiles;
   std::string igmFileName("");
   std::string bilFileName("");
   std::string objFileName("");
   std::string fodisFileName("");
   std::string templateOutputFileName("");
   std::string templateType("");
   gmtl::Vec3i templateSize;
   std::string templatesImagePlot("");
   std::string exportVolumeFileName("");
   bool volumeCompression = false;
   std::string volumeFileName("");
   std::string dtmFileName("");
   std::vector<std::string> pwFiles;
   std::string volumeType("hashed_1D_array");
   double voxelLength = 2.5f;
   double noiseLevel = 25.0f;
   double isolevel = 0.0f;
   std::string mapsAll("");
   std::string csvSamples("");

   // TYPES:
   // 0. Full-waveform
   // 1. All_Discrete
   // 2. Discrete_n_Waveform
   // 3. Discrete (associated with waveform only
   std::string fileType("full-waveform");
//   type = "AGC";

   std::vector<short unsigned int > bands(3);
   bands[0] = 140;
   bands[1] = 78;
   bands[2] = 23;
   std::vector<MapsManager::mapInfo *> mInfo;

   MapsManager m;
   unsigned int mapsIndex=0;
   bool intergalVolume = false;

   std::vector<double> userLimits(6, 0.0f);

   // PARSING
   // pair arguments to numbers to ease search
   std::map<std::string, int> tags;

   // igm file name of hyperspectral imagery - string
   tags["-igm"] = 1; /// -igm <filename>
   // bil file name of hyperspectral imagery - string
   tags["-bil"] = 2; // <filename>
   // export polygon mesh to an obj file - string
   tags["-obj"] = 3; /// -obj <filename>
   // the bands of the hyperspectral imagery that will be used as RGB values - int int int
   tags["-rgb"] = 4; /// -rgb <band1> <band2> <band3>
   // the voxel length - double
   tags["-vl" ] = 5; /// -vl <voxel length>
   // the noise level - double
   tags["-nl" ] = 6; /// -nl <noise level>
   // isolevel
   tags ["-iso"] = 7;
   // type of the object, what to import in the polygon mesh
   // - 0. full waveform
   // - 1. All Discrete
   // - 2. Discrete and Waveform
   // - 3. Discrete (only associated with the waveform)
   // - 4. AGC value
   tags ["-otype"] = 8;
   // generate a map - 1.type of map - 2. possible more parameters - 3. name of file to be exported
   //  1. types of maps:
   // ["NON-EMPTY VOXELS"] = 1;
   // ["DENSITY"]   = 2;
   // ["THICKNESS"]    = 3;
   // ["HYPERSPECTRAL"] = 4; - follwed by band number
   // ["HYPERSPECTRAL MEAN"] = 5;
   // ["LOWEST_RETURN"] = 6;
   // 2. more parameters
   // -thres <integer from [1-255] to threshold the map
   tags ["-map"] = 9; /// -map  <type>  ?<band> ?{ -thres <threshold> }  <output_name>  ?{ -signature <type> <signature_directory> }
   // optimisation algorithm - on/off
   tags ["-opt"] = 10; /// -opt <on/off>;

   // print instructions on how to use the software
   tags ["--help"] = 12;
   // LAS file name - string
   tags["-las"] = 13; /// -las <lasFileName>
   // load fodis file
   tags ["-fodis"] = 14;
   // fieldplot
   tags ["-fieldplot"] = 15;
   // export volume? on or off - by default its off
   // 'c' indicate compression and its optional. Not every data structure has this functionality
   tags ["-exportVolume"] = 16; /// -export_volume <'c'> <exportedVolumeFileName>;
   // imports a volume instead of a las file
   tags ["-volume"] = 17; /// -volume <volumeFileName>
   // imports a pulsewave file
   tags ["-pw"] = 18; /// -pw <pwFileName>
   // define the limits of the area of interest [0:MaxNorthY, 1:MinNorthY, 2:MaxEastX, 3:MinEastX, 4:MaxHeightZ, 5:MinHeightZ]
   tags ["-userLimits"] = 19; /// -userLimits <MaxNorthY> <MinNorthY> <MaxEastX> <MinEastX>
   // creates templates , type = svm, rd or nn
   tags ["-templates"] = 20; /// -templates <type> <sizeX> <sizeY> <sizeZ> <input_fieldPlot_image> <output_templatesName>
   // choose type of structure, types = 1D_ARRAY HASHED_1D_ARRAY OCTREE  INTEGRAL_VOLUMES INTEGRAL_TREE  HASHED_OCTREE SERIES_OF_HASHED_OCTREES
   tags["-stype"] = 21; /// -stype <structure_type>
   // used to make surface flat
   tags["-dtm"]= 22; /// -dtm <dtm_fileName>
   // exports 10 samples into a .csv file
   tags["-pulseSamples"]=23;  /// -pulseSamples <filename.csv>


   try
   {
     int argvIndex = 1;
     while(argvIndex<argc)
     {
        switch (tags[argv[argvIndex]])
        {
        case 1: // -igm <filename>
        {
           argvIndex ++;
           if (argvIndex<argc)
           {
              igmFileName = argv[argvIndex];
           }
           break;
        }
        case 2: // -bil <filename>
        {
           argvIndex ++;
           if (argvIndex<argc)
           {
              bilFileName = argv[argvIndex];
           }
           break;
        }
        case 3: // -obj <filename>
        {
           argvIndex ++;
           if (argvIndex<argc)
           {
              objFileName = argv[argvIndex];
           }
           break;
        }
        case 4: // -rgb <band1> <band2> <band3>
        {
           argvIndex+=3;
           if (argvIndex<argc)
           {
              bands[0] = atoi(argv[argvIndex-2]);
              bands[1] = atoi(argv[argvIndex-1]);
              bands[2] = atoi(argv[argvIndex  ]);
           }
           break;
        }
        case 5: // -vl <voxel length>
        {
           argvIndex++;
           if (argvIndex<argc)
           {
              voxelLength = atof(argv[argvIndex]);
           }
           break;
        }
        case 6: // -nl <noise level>
        {
           argvIndex++;
           if (argvIndex<argc)
           {
              noiseLevel = atof(argv[argvIndex]);
           }
           break;
        }
        case 7: // -iso <isolevel>
        {
           argvIndex++;
           if (argvIndex<argc)
           {
              isolevel = atof(argv[argvIndex]);
           }
           break;
        }
        case 8: // -otype <input data type>
        {
            argvIndex++;
            if (argvIndex<argc)
            {
               fileType = argv[argvIndex];
            }
            break;
        }
        case 9: // -map  <type>  ?<band> ?{ -thres <threshold> } ?<fieldplot_name> <output_name>  ?{ -signature <type> <signature_directory> }
        {
           argvIndex+=2;
           if(argvIndex>=argc)
           {
              std::cout << "WARNING: map does not have enough parameters\n";
              break;
           }


           // add all metrics to mInfo later, once all parameters are defined

           std::string s(argv[argvIndex-1]);
           for (auto & c: s) c = std::toupper(c);
           if(s=="ALL_FW")
           {
              mapsAll= std::string(argv[argvIndex]) + "_";
              break;
           }

           mInfo.push_back(new MapsManager::mapInfo);
           mInfo[mapsIndex]->type = s;
           if(s=="HYPERSPECTRAL")
           {
              argvIndex++;
              if(argvIndex<argc)
              {
                 mInfo[mapsIndex]->band = atoi(argv[argvIndex-1]);
              }
              else
              {
                  std::cout << "WARNING: Band haven't been defined. Set to 140 as default";
                  mInfo[mapsIndex]->band = 140;
                  break;
              }
           }



           mInfo[mapsIndex]->thres = 0;
           if(argvIndex+2<argc && std::string(argv[argvIndex])=="-thres")
           {
              argvIndex+=2;
              mInfo[mapsIndex]->thres = atoi(argv[argvIndex-1]);
           }

           if(s=="FIELDPLOT" && argvIndex+1<argc)
           {
              break;
           }

           // <output_name>
           mInfo[mapsIndex]->name = argv[argvIndex];

           if(s=="SPECTRAL_SIGNATURE")
           {
              if(argvIndex+3<argc && std::string(argv[argvIndex+1])=="-signature")
              {
                 argvIndex+=3;
                 mInfo[mapsIndex]->spectralSignatureType = argv[argvIndex-2]; // "ASTER" or "notASTER"
                 mInfo[mapsIndex]->spectralSignature = argv[argvIndex-1];
                 break;
              }
           }
           // mInfo[mapsIndex]->obj , defined later
           mInfo[mapsIndex]->bilFileName = bilFileName;
           mInfo[mapsIndex]->IGMfileName = igmFileName;
           mInfo[mapsIndex]->fodisFileName =fodisFileName;
           mInfo[mapsIndex]->samp = 0;
           mapsIndex++;
           break;
        }
        case 10: // -opt <on/off>;
        {
           argvIndex++;
           if (argvIndex<argc)
           {
              intergalVolume = (!(std::string(argv[argvIndex])=="off"));
           }
           break;
        }
        case 12: // "--help"
        {
           std::fstream readmeFile;
           readmeFile.open("README.txt");
           if(!readmeFile.is_open())
           {
              std::cerr<<"ERROR: README.txt file doesn't exist.\n"
                       <<"Please look at previous versions of DAOS for instructions\n";
              return EXIT_FAILURE;
           }
           std::string line;
           while(std::getline(readmeFile,line))
           {
              std::cout << line << "\n";
           }
           return EXIT_SUCCESS;
           break;
        }
        case 13: // -las
        {        
           argvIndex++;
           while (argvIndex<argc)
           {
              if(argv[argvIndex][0]!='-')
              {
                 lasFiles.push_back(std::string(argv[argvIndex]));
                 argvIndex++;
              }
              else
              {
                 argvIndex--;
                 break;
              }
           }
           break;
        }
        case 14: // -fodis <filename>
        {
           argvIndex ++;
           if (argvIndex<argc)
           {
              fodisFileName = argv[argvIndex];
           }
           break;
        }
        case 15: // -fieldplot <filename>
        {
           argvIndex ++;
           if (argvIndex<argc)
           {
           }
           break;
        }
        case 16: // -exportVolume <'c'> <exportedVolumeFileName>;
        {
           argvIndex++;
           if (argvIndex<argc)
           {
              if(std::string(argv[argvIndex])!="c")
              {
                 exportVolumeFileName = argv[argvIndex];
              }
              else // compression is enable if available
              {
                  argvIndex++;
                  volumeCompression=true;
                  if(argvIndex<argc)
                  {
                     exportVolumeFileName=argv[argvIndex];
                  }
              }
           }
           break;
        }
        case 17: // -volume <volumeFileName>
        {
           argvIndex++;
           if (argvIndex<argc)
           {
              volumeFileName = argv[argvIndex];
           }
           break;
        }
        case 18: // -pw <pulsewaveFileName>
        {
           argvIndex++;
           while (argvIndex<argc)
           {
              if(argv[argvIndex][0]!='-')
              {
                 pwFiles.push_back(std::string(argv[argvIndex]));
                 argvIndex++;
              }
              else
              {
                 argvIndex--;
                 break;
              }
           }
           break;
        }
        case 19: // -userLimits <MaxNorthY> <MinNorthY> <MaxEastX> <MinEastX>
        {
           argvIndex+=4;
           if (argvIndex<argc)
           {
              userLimits[3] = atof(argv[argvIndex  ]);
              userLimits[2] = atof(argv[argvIndex-1]);
              userLimits[1] = atof(argv[argvIndex-2]);
              userLimits[0] = atof(argv[argvIndex-3]);
           }
           break;
        }
        case 20: // -templates <type> <sizeX> <sizeY> <sizeZ> <input_fieldPlot_image> <output_templatesName>
        {
           argvIndex+=6;
           if(argvIndex<argc)
           {
              templateOutputFileName = argv[argvIndex  ] ;
              templatesImagePlot      = argv[argvIndex-1] ;
              templateSize[2]   = atof(argv[argvIndex-2]);
              templateSize[1]   = atof(argv[argvIndex-3]);
              templateSize[0]   = atof(argv[argvIndex-4]);
              templateType      =      argv[argvIndex-5] ;
           }
           break;
        }
        case 21:// -stype <structure_type>
        {
            argvIndex++;
            if (argvIndex<argc)
            {
               volumeType = argv[argvIndex];
            }

            break;
        }
        case 22: // -dtm <dtm_filename>
        {
           argvIndex ++;
           if (argvIndex<argc)
           {
              std::string dtmName(argv[argvIndex]);
              if(dtmName.substr(dtmName.find_last_of(".") + 1) == "bil")
              {
                 dtmFileName = dtmName;
              }
              else
              {
                std::cerr<<"WARNING: DTM file ignore. Not supported format.\n";
              }
           }
           break;
        }
        case 23: // -pulseSamples <filename.csv>
        {
            argvIndex++;
            if (argvIndex<argc)
            {
               csvSamples = argv[argvIndex];
            }
            break;
        }
        default:
        {
           std::cout << "WARNING: Unkown tag: " << argv[argvIndex] << "\n";
           break;
        }
        }
        argvIndex++;
     }
    }
    catch (char const* e)
    {
       std::cout << e  << std::endl;
       std::cout << "Type Las1.3Vis --help and follow the instructions\n";
       std::this_thread::sleep_for(std::chrono::milliseconds(20000));
       return EXIT_FAILURE;
    }

   //INTERPRETATION OF DATA


   Volume *vol = NULL;

   //--------------------------------------------------------------------------------------
   // read filename
   //--------------------------------------------------------------------------------------
   if(lasFiles.size()==0 && volumeFileName=="" && pwFiles.size()==0)
   {
      std::cerr << "LAS, pulsewave or volume file haven't been specified\n";
      std::cerr << "use \"DASOS -- help\" for instructions.\n";
      return EXIT_FAILURE;
   }

   if(lasFiles.size()!=0)
   {
      Las1_3_handler lala(lasFiles[0]);
      if(userLimits[0]<0.001 && userLimits[0]>-0.0001)
      {
         //then user haven't defined limits
         std::cout << "WARNING: Limits haven't been set, so entire file will be loaded\n";
         userLimits = lala.getBoundaries();
      }
      else
      {
         std::vector<double> temp_userLimits(lala.getBoundaries());
         userLimits[4] = temp_userLimits[4];
         userLimits[5] = temp_userLimits[5];
      }
      std::cout << "userLimits "<< userLimits[0] << " " << userLimits[1]<<" "
                << userLimits[2]<<" " << userLimits[0]<<"\n";

      vol = VolumeFactory::produceVolume(voxelLength,userLimits,volumeType);
      vol->setNoiseLevel(noiseLevel);
      lala.readFileAndGetObject(vol,fileType,dtmFileName);
      for(unsigned int nextFile=1; nextFile<lasFiles.size(); ++nextFile)
      {
         Las1_3_handler nextLAShandler(lasFiles[nextFile]);
         nextLAShandler.readFileAndGetObject(vol,fileType,dtmFileName);
      }
      vol->normalise();

   }
   else if (volumeFileName!="")
   {
      vol = VolumeFactory::produceVolume(volumeFileName,volumeType);
      gmtl::Vec3f maxLimits = vol->getMaxLimits();
      gmtl::Vec3f minLimits = vol->getMinLimits();
      userLimits[0] = maxLimits[1];
      userLimits[1] = minLimits[1];
      userLimits[2] = maxLimits[0];
      userLimits[3] = minLimits[0];
      userLimits[4] = maxLimits[2];
      userLimits[5] = maxLimits[2];
      vol->setIsolevel(isolevel);
   }
   else if (pwFiles.size()!=0)
   {
      PW_handler pw(pwFiles[0]);
      if(userLimits[0]<0.001 && userLimits[0]>-0.0001)
      {
         //then user haven't defined limits
         userLimits = pw.getBoundaries();
         std::cout << "WARNING: Limits haven't been set, so entire file will be loaded\n";
         std::cout << "userLimits " << userLimits[0] << " " << userLimits[1] << " " << userLimits[2] << " " << userLimits[3] <<" " << userLimits[4] << " " << userLimits[5] << "\n";
      }
      else
      {
         std::vector<double> temp_userLimits(pw.getBoundaries());
         userLimits[4] = temp_userLimits[4];
         userLimits[5] = temp_userLimits[5];
      }
      vol = VolumeFactory::produceVolume(voxelLength,userLimits,volumeType);
      pw.readFileAndGetObject(vol,"full-waveform",dtmFileName);
      for(unsigned int nextFile=1; nextFile<pwFiles.size(); ++nextFile)
      {
         PW_handler nextPWhandler(pwFiles[nextFile]);
         nextPWhandler.readFileAndGetObject(vol,"full-waveform",dtmFileName);
      }

      vol->normalise();
      std::cout << "Pulse WAVES read!\n";
   }

   vol->setIsolevel(isolevel);
   vol->setNoiseLevel(noiseLevel);

   if(exportVolumeFileName!="")
   {
     vol->exportToFile(exportVolumeFileName,volumeCompression);
   }


   //--------------------------------------------------------------------------------------
   // Polygonise volume
   //--------------------------------------------------------------------------------------
   if(objFileName!="")
   {
      GLData *glData = new GLData;
      MarchingCubes *mc = VolumeFactory::getMarchingCubes(
                  volumeType,intergalVolume,vol,ceil((userLimits[2]-userLimits[3])/voxelLength)
              );
      mc->createPolygonisedObject(glData);

      if(igmFileName!="")
      {
         glData->createUVsIGM(igmFileName);
         std::cout << "UVs created\n";
      }

      glData->exportToObj(objFileName,igmFileName,bilFileName,bands);
      if (glData!=NULL)
      {
         delete glData;
      }
      if (mc!=NULL)
      {
         delete mc;
      }
    }

   //--------------------------------------------------------------------------------------
   // generate maps
   //--------------------------------------------------------------------------------------
   // if mapsAll then add all the fw maps to mapInfo
   if(mapsAll!="")
   {
      MapsManager *m;
      m = new MapsManager;
      const std::vector<std::string> &metricsNames = m->getNamesOfFWMetrics();
      unsigned int noOfMetrics=metricsNames.size();
      for(unsigned int i=0; i<noOfMetrics;++i)
      {
          mInfo.push_back(new MapsManager::mapInfo);
          mInfo[mInfo.size()-1]->type = metricsNames[i];
          mInfo[mInfo.size()-1]->name = mapsAll + metricsNames[i];
          mInfo[mInfo.size()-1]->band = 140;
          mInfo[mInfo.size()-1]->thres = 0;
          mInfo[mInfo.size()-1]->samp = 0;
          ++mapsIndex;
      }
      delete m;
   }
   for(unsigned int i=0; i<mInfo.size(); ++i)
   {
      mInfo[i]->obj = vol;
      m.createMap(mInfo[i]);
      delete mInfo[i];
   }
   if(mInfo.size()!=0)
   {
      std::cout << "ALL MAPS Saved\n";
   }

   if(vol!=NULL)
   {
      delete vol;
   }
   std::cout << "   ***   EXIT   ***\n";
   return 0;
}
