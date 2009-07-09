#include "DiskIdx2UbyteDataSet.h"
#include "IOBufferize.h"
#include "IOAscii.h"
#include "IOMulti.h"
#include "IOBin.h"
#include "IOSub.h"
#include "IOIdx2Ubyte.h"

namespace Torch {

DiskIdx2UbyteDataSet::DiskIdx2UbyteDataSet(const char *filename, int n_inputs_, int n_targets_,
                               bool one_file_is_one_sequence, int max_load, bool binary_mode)
{
  if( (n_inputs_ < 0) && (n_targets < 0) )
    error("DiskIdx2UbyteDataSet: cannot guess n_inputs <and> n_targets!");

  IOSequence *io_file = NULL;
  if(binary_mode)
    io_file = new(allocator) IOIdx2Ubyte(filename, one_file_is_one_sequence, max_load, false);
  else
    io_file = new(allocator) IOAscii(filename, one_file_is_one_sequence, max_load);

  init_(io_file, n_inputs_, n_targets_);
}


void DiskIdx2UbyteDataSet::init_(IOSequence *io_file, int n_inputs_, int n_targets_)
{
  if( (n_inputs_ > io_file->frame_size) || (n_targets_ > io_file->frame_size) )
    error("DiskIdx2UbyteDataSet: n_inputs (%d) or n_targets (%d) too large (> %d) !", n_inputs_, n_targets_, io_file->frame_size);

  if(n_inputs_ < 0)
    n_inputs_ = io_file->frame_size - n_targets_;

  if(n_targets_ < 0)
    n_targets_ = io_file->frame_size - n_inputs_;

  if(io_file->frame_size != (n_inputs_ + n_targets_))
    error("DiskIdx2UbyteDataSet: %d columns in the file != %d inputs + %d targets", io_file->frame_size, n_inputs_, n_targets_);

  IOBufferize *io_buffer = NULL;
  if( (n_inputs_ > 0) && (n_targets_ > 0) )
    io_buffer = new(allocator) IOBufferize(io_file);

  if(n_inputs_ > 0)
  {
    if(n_targets_ > 0)
      io_inputs = new(allocator) IOSub(io_buffer, 0, n_inputs_);
    else
      io_inputs = io_file;
  }
  if(n_targets_ > 0)
  {
    if(n_inputs_ > 0)
      io_targets = new(allocator) IOSub(io_buffer, n_inputs_, n_targets_);
    else
      io_targets = io_file;
  }

  DiskDataSet::init(io_inputs, io_targets);

  message("DiskIdx2UbyteDataSet: %d examples scanned", n_examples);
}

DiskIdx2UbyteDataSet::~DiskIdx2UbyteDataSet()
{
}


DiskIdx2UbyteDataSet::DiskIdx2UbyteDataSet(char **filenames, int n_files_, int n_inputs_, int n_targets_,
                               bool one_file_is_one_sequence, int max_load, bool binary_mode)
{
  if(n_files_ <= 0)
    error("DiskIdx2UbyteDataSet: check the number of files!");

  IOSequence **io_files = (IOSequence **)allocator->alloc(sizeof(IOSequence *)*n_files_);
  if(max_load > 0)
  {
    int i = 0;
    while( (max_load > 0) && (i < n_files_) )
    {
      if(binary_mode)
        io_files[i] = new(allocator) IOIdx2Ubyte(filenames[i], one_file_is_one_sequence, max_load, false);
      else
        io_files[i] = new(allocator) IOAscii(filenames[i], one_file_is_one_sequence, max_load);
      max_load -= io_files[i]->n_sequences;
      i++;
    }
    n_files_ = i;
  }
  else
  {
    if(binary_mode)
    {
      for(int i = 0; i < n_files_; i++)
        io_files[i] = new(allocator) IOIdx2Ubyte(filenames[i], one_file_is_one_sequence, -1, false);
    }
    else
    {
      for(int i = 0; i < n_files_; i++)
        io_files[i] = new(allocator) IOAscii(filenames[i], one_file_is_one_sequence, -1);
    }
  }

  IOMulti *io_file = new(allocator) IOMulti(io_files, n_files_);
  init_(io_file, n_inputs_, n_targets_);
}

DiskIdx2UbyteDataSet::DiskIdx2UbyteDataSet(char **input_filenames, char **target_filenames, int n_files_,
                               int max_load, bool binary_mode)
{
  if(n_files_ <= 0)
    error("DiskIdx2UbyteDataSet: check the number of files!");

  if(input_filenames)
  {
    IOSequence **input_io_files = (IOSequence **)allocator->alloc(sizeof(IOSequence *)*n_files_);
    int max_load_ = max_load;
    int n_files__ = 0;
    if(max_load_ > 0)
    {
      int i = 0;
      while( (max_load_ > 0) && (i < n_files_) )
      {
        if(binary_mode)
          input_io_files[i] = new(allocator) IOIdx2Ubyte(input_filenames[i], true, max_load_);
        else
          input_io_files[i] = new(allocator) IOAscii(input_filenames[i], true, max_load_);
        max_load_ -= input_io_files[i]->n_sequences;
        i++;
      }
      n_files__ = i;
    }
    else
    {
      if(binary_mode)
      {
        for(int i = 0; i < n_files_; i++)
          input_io_files[i] = new(allocator) IOIdx2Ubyte(input_filenames[i], true, -1);
      }
      else
      {
        for(int i = 0; i < n_files_; i++)
          input_io_files[i] = new(allocator) IOAscii(input_filenames[i], true, -1);
      }
      n_files__ = n_files_;
    }
    io_inputs = new(allocator) IOMulti(input_io_files, n_files__);
  }

  if(target_filenames)
  {
    IOSequence **target_io_files = (IOSequence **)allocator->alloc(sizeof(IOSequence *)*n_files_);
    int max_load_ = max_load;
    int n_files__ = 0;
    if(max_load_ > 0)
    {
      int i = 0;
      while( (max_load_ > 0) && (i < n_files_) )
      {
        if(binary_mode)
          target_io_files[i] = new(allocator) IOIdx2Ubyte(target_filenames[i], true, max_load_);
        else
          target_io_files[i] = new(allocator) IOAscii(target_filenames[i], true, max_load_);
        max_load_ -= target_io_files[i]->n_sequences;
        i++;
      }
      n_files__ = i;
    }
    else
    {
      if(binary_mode)
      {
        for(int i = 0; i < n_files_; i++)
          target_io_files[i] = new(allocator) IOIdx2Ubyte(target_filenames[i], true, -1);
      }
      else
      {
        for(int i = 0; i < n_files_; i++)
          target_io_files[i] = new(allocator) IOAscii(target_filenames[i], true, -1);
      }
      n_files__ = n_files_;
    }
    io_targets = new(allocator) IOMulti(target_io_files, n_files__);
  }

  DiskDataSet::init(io_inputs, io_targets);
  message("DiskIdx2UbyteDataSet: %d examples scanned [%d inputs and %d targets detected]", n_examples, n_inputs, n_targets);
}


}
