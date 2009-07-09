
#ifndef DISK_IDX2UBYTE_DATA_SET_INC
#define DISK_IDX2UBYTE_DATA_SET_INC

#include "DiskDataSet.h"

namespace Torch {

class DiskIdx2UbyteDataSet : public DiskDataSet
{
  private:
    void init_(IOSequence *io_file, int n_inputs_, int n_targets_);

  public:
    DiskIdx2UbyteDataSet(const char *filename, int n_inputs_, int n_targets_, bool one_file_is_one_sequence=false, int max_load=-1, bool binary_mode=true);
    DiskIdx2UbyteDataSet(char **filenames, int n_files_, int n_inputs_, int n_targets_, bool one_file_is_one_sequence=false, int max_load=-1, bool binary_mode=true);
    DiskIdx2UbyteDataSet(char **input_filenames, char **target_filenames, int n_files_, int max_load=-1, bool binary_mode=true);
    virtual ~DiskIdx2UbyteDataSet();
};

}

#endif
