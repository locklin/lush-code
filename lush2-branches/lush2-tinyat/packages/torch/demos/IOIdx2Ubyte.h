#ifndef IO_IDX2UBYTE_INC
#define IO_IDX2UBYTE_INC

#include "IOSequence.h"
#include "DiskXFile.h"

namespace Torch {


class IOIdx2Ubyte : public IOSequence
{
  private:
    DiskXFile *file;
    int current_frame_index;

  public:
    bool one_file_is_one_sequence;
    int n_total_frames;
    char *filename;
    int max_load;
    bool is_sequential;

    IOIdx2Ubyte(const char *filename_, bool one_file_is_one_sequence_=false, int max_load_=-1, bool is_sequential=true);
    static void saveSequence(XFile *file, Sequence *sequence);
    virtual void getSequence(int t, Sequence *sequence);
    virtual int getNumberOfFrames(int t);
    virtual int getTotalNumberOfFrames();
    virtual ~IOIdx2Ubyte();
};

}

#endif
