
#include "IOIdx2Ubyte.h"

namespace Torch {

void IOIdx2Ubyte::saveSequence(XFile *file, Sequence *sequence)
{
  file->write(&sequence->n_frames, sizeof(int), 1);
  file->write(&sequence->frame_size, sizeof(int), 1);

  for(int i = 0; i < sequence->n_frames; i++)
    file->write(sequence->frames[i], sizeof(real), sequence->frame_size);
}

IOIdx2Ubyte::IOIdx2Ubyte(const char *filename_, bool one_file_is_one_sequence_, int max_load_, bool is_sequential_)
{
  one_file_is_one_sequence = one_file_is_one_sequence_;
  max_load = max_load_;
  is_sequential = is_sequential_;

  filename = (char *)allocator->alloc(strlen(filename_)+1);
  strcpy(filename, filename_);

  int fileformat_flag;
  int num_dim;
  int junk;
  DiskXFile f(filename, "r");
  f.read(&fileformat_flag, sizeof(int), 1);
  f.read(&num_dim, sizeof(int), 1);
  f.read(&n_total_frames, sizeof(int), 1);
  f.read(&frame_size, sizeof(int), 1);
  f.read(&junk, sizeof(int), 1);

  if( (max_load > 0) && (max_load < n_total_frames) && (!one_file_is_one_sequence) )
  {
    n_total_frames = max_load;
    message("IOIdx2Ubyte: loading only %d frames", n_total_frames);
  }

  if(one_file_is_one_sequence)
    n_sequences = 1;
  else
    n_sequences = n_total_frames;
  
  file = NULL;
  current_frame_index = -1;  
}

void IOIdx2Ubyte::getSequence(int t, Sequence *sequence)
{
  unsigned char *frame_buffer = (unsigned char *)allocator->alloc(frame_size);
  if(one_file_is_one_sequence)
  {
    file = new(allocator) DiskXFile(filename, "r");
    int murielle;
    int fileformat_flag;
    file->read(&fileformat_flag, sizeof(int), 1);
    file->read(&murielle, sizeof(int), 1);  // ndim
    file->read(&murielle, sizeof(int), 1);  // dim[0]
    file->read(&murielle, sizeof(int), 1);  // dim[1]
    file->read(&murielle, sizeof(int), 1);  // dim[2]
    for(int i = 0; i < n_total_frames; i++)
      {
	file->read(frame_buffer, sizeof(unsigned char), frame_size);
	for(int j =0; j< frame_size; j++)
	  sequence->frames[i][j] = (real) frame_buffer[j];
      }
    //      file->read(sequence->frames[i], sizeof(real), frame_size);
    allocator->free(file);
  }
  else
  {
    if(is_sequential)
    {
      if(t != current_frame_index+1)
        error("IOIdx2Ubyte: sorry, data are accessible only in a sequential way");
      
      if(current_frame_index < 0)
      {
        file = new(allocator) DiskXFile(filename, "r");
        int murielle;
	file->read(&murielle, sizeof(int), 1);  // magic
	file->read(&murielle, sizeof(int), 1);  // ndim
	file->read(&murielle, sizeof(int), 1);  // dim[0]
        file->read(&murielle, sizeof(int), 1);  // dim[1]
        file->read(&murielle, sizeof(int), 1);  // dim[2]
      }
    }
    else
    {
      file = new(allocator) DiskXFile(filename, "r");
      // if(file->seek(t*frame_size*sizeof(real)+2*sizeof(int), SEEK_CUR) != 0)
      if(file->seek(t * frame_size * sizeof(unsigned char) + 5 * sizeof(int), SEEK_CUR) !=0)
        error("IOIdx2Ubyte: cannot seek in your file!");
    }

    // file->read(sequence->frames[0], sizeof(real), frame_size);
    file->read(frame_buffer, sizeof(unsigned char), frame_size);
    for(int j =0; j<frame_size; j++)
      sequence->frames[0][j] = (real) frame_buffer[j];

    if(is_sequential)
    {
      current_frame_index++;
      if(current_frame_index == n_total_frames-1)
      {
        allocator->free(file);
        current_frame_index = -1;
      }
    }
    else
      allocator->free(file);
  }
  allocator->free(frame_buffer);
}

int IOIdx2Ubyte::getNumberOfFrames(int t)
{
  if(one_file_is_one_sequence)
    return n_total_frames;
  else
    return 1;
}

int IOIdx2Ubyte::getTotalNumberOfFrames()
{
  return n_total_frames;
}

IOIdx2Ubyte::~IOIdx2Ubyte()
{
}

}
