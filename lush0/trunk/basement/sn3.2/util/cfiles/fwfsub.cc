
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


// ------------------------------- #include "errors.h"

/*

    Ce module decrit toutes les erreurs de dos et des
    differents objets. (et eventuellement unix, winnt 
    et les autres)

*/


/* 

   Some of the messages are only warnings. They are not errors
   but only some informative token. 
   
   The crital errors are far more serious. On some operating
   system, they might indicate an IMMINENT system crash (like
   DOS or win3.1 or anything that bad) while the other errors 
   are recoverable in some way, or already have been recovered.
   
*/
   

const int none                                             =0;

/* Turbo pascal runtime errors & dos errors */

const int       rte_invalid_function_number                =1;
const int       rte_file_not_found                         =2;
const int       rte_path_not_found                         =3;
const int       rte_too_many_files_opened                  =4;
const int       rte_file_access_denied                     =5;
const int       rte_invalid_file_handle                    =6;
const int       dos_memory_control_block_destroyed         =7;
const int       dos_not_enough_memory                      =8;
const int       dos_invalid_memory_block_address           =9;
const int       dos_invalid_environment_block              =10;
const int       dos_invalid_format                         =11;
const int       rte_invalid_file_access_code               =12;
const int       dos_invalid_data                           =13;
const int       dos_reserved_14                            =14;
const int       rte_invalid_drive_number                   =15;
const int       rte_cannot_remove_current_directory        =16;
const int       rte_cannot_rename_accros_drives            =17;
const int       dos_no_more_files                          =18;
const int       dos_disk_is_write_protected                =19;
const int       dos_unknow_disk_unit                       =20;
const int       dos_drive_not_ready                        =21;
const int       dos_unknow_disk_command                    =22;
const int       dos_disk_data_error                        =23;
const int       dos_bad_disk_request                       =24;
const int       dos_disk_seek_error                        =25;
const int       dos_non_dos_disk                           =26;
const int       dos_sector_not_found                       =27;
const int       dos_printer_out_of_paper                   =28;
const int       dos_write_error                            =29;
const int       dos_read_error                             =30;
const int       dos_general_failure                        =31;
const int       dos_file_sharing_violation                 =32;
const int       dos_file_locking_violation                 =33;
const int       dos_file_invalid_disk_change               =34;
const int       dos_no_fcb_available                       =35;
const int       dos_sharing_buffer_overflow                =36;
const int       dos_reserved_37                            =37;
const int       dos_reserved_38                            =38;
const int       dos_reserved_39                            =39;
const int       dos_reserved_40                            =40;
const int       dos_reserved_41                            =41;
const int       dos_reserved_42                            =42;
const int       dos_reserved_43                            =43;
const int       dos_reserved_44                            =44;
const int       dos_reserved_45                            =45;
const int       dos_reserved_46                            =46;
const int       dos_reserved_47                            =47;
const int       dos_reserved_48                            =48;
const int       dos_reserved_49                            =49;
const int       dos_netword_request_not_supported          =50;
const int       dos_remote_computer_not_listening          =51;
const int       dos_duplicate_name_on_netword              =52;
const int       dos_network_name_not_found                 =53;
const int       dos_network_busy                           =54;
const int       dos_netword_device_is_dead                 =55;
const int       dos_netword_bios_command_limit_exceeded    =56;
const int       dos_netword_adapter_error                  =57;
const int       dos_incorrect_reponse_from_network         =58;
const int       dos_unexpected_network_error               =59;
const int       dos_incompatible_remote_adapter            =60;
const int       dos_print_queue_full                       =61;
const int       dos_not_enough_space_for_print_file        =62;
const int       dos_print_file_was_deleted                 =63;
const int       dos_netword_name_was_deleted               =64;
const int       dos_access_denied                          =65;
const int       dos_network_type_incorrect                 =66;
const int       dos_network_name_not_found2                =67;
const int       dos_network_name_limit_exceeded            =68;
const int       dos_net_bios_limit_exceeded                =69;
const int       dos_sharing_paused                         =70;
const int       dos_network_request_refused                =71;
const int       dos_redirection_paused                     =72;
const int       dos_reserved_73                            =73;
const int       dos_reserved_74                            =74;
const int       dos_reserved_75                            =75;
const int       dos_reserved_76                            =76;
const int       dos_reserved_77                            =77;
const int       dos_reserved_78                            =78;
const int       dos_reserved_79                            =79;
const int       dos_file_already_exist                     =80;
const int       dos_reserved_81                            =81;
const int       dos_cannot_create_directory_entry          =82;
const int       dos_fail_on_interrupt_24                   =83;
const int       dos_out_of_network_structure               =84;
const int       dos_net_device_already_assigned            =85;
const int       dos_invalid_password                       =86;
const int       dos_invalid_parameter                      =87;
const int       dos_network_data_fault                     =88;
const int       dos_reserved_89                            =89;
const int       dos_reserved_90                            =90;
const int       dos_reserved_91                            =91;
const int       dos_reserved_92                            =92;
const int       dos_reserved_93                            =93;
const int       dos_reserved_94                            =94;
const int       dos_reserved_95                            =95;
const int       dos_reserved_96                            =96;
const int       dos_reserved_97                            =97;
const int       dos_reserved_98                            =98;
const int       dos_reserved_99                            =99;
const int       rte_disk_read_error                        =100;
const int       rte_disk_write_error                       =101;
const int       rte_file_not_assigned                      =102;
const int       rte_file_not_open                          =103;
const int       rte_file_not_for_input                     =104;
const int       rte_file_not_for_output                    =105;
const int       rte_invalid_numeric_format                 =106;

const int       rte_disk_is_write_protected                =150;
const int       rte_unknown_unit                           =151;
const int       rte_drive_not_ready                        =152;
const int       rte_unknown_command                        =153;
const int       rte_crc_error_in_data                      =154;
const int       rte_bad_drive_request_structure_length     =155;
const int       rte_disk_seek_error                        =156;
const int       rte_unknown_media_type                     =157;
const int       rte_sector_not_found                       =158;
const int       rte_printer_out_of_paper                   =159;
const int       rte_device_write_fault                     =160;
const int       rte_device_read_fault                      =161;
const int       rte_hardware_failure                       =162;

const int       rte_division_by_zero                       =200;
const int       rte_range_check_error                      =201;
const int       rte_stack_overflow                         =202;
const int       rte_heap_overflow                          =203;
const int       rte_invalid_pointer_operation              =204;
const int       rte_floating_point_overflow                =205;
const int       rte_floating_point_underflow               =206;
const int       rte_invalid_floating_point_operation       =207;
const int       rte_overlay_manager_not_installed          =208;
const int       rte_overlay_file_read_error                =209;
const int       rte_object_not_initialized                 =210;
const int       rte_call_to_abstract                       =211;
const int       rte_stream_registration_error              =212;
const int       rte_collection_index_range                 =213;
const int       rte_collection_overflow                    =214;
const int       rte_arithmetic_overflow_error              =215;
const int       rte_general_protection_fault               =216;



/* main block of user-defined errors for lists, etc.  */


const int      access_denied                              =256;    /* The process/object has unsufficient priviledge to touch the object */
const int      access_need_password                       =257;    /* The process/object needs a validated password to touch the object */
const int      bad_control_byte                           =258;    /* The control byte is illegal (wrong enumerated type, bit pattern, etc. */
const int      bad_control_longint                        =259;    /* The control int (32) is illegal (wrong enumerated type, bit pattern, etc. */
const int      bad_control_word                           =260;    /* The control int (16) is illegal (wrong enumerated type, bit pattern, etc. */
const int      buffer_empty                               =261;    /* Warning: the current buffer is empty */
const int      buffer_full                                =262;    /* Warning: the current buffer is full; last 'put' has failed */

const int      cannot_overwrite_color_0                   =263;    /* Blitter operation failed on color0 ("background") */

const int      cant_find_resource                         =264;    /* The file/resource could not be found */
const int      cant_load_resource                         =265;    /* The file/resource was found but could not be loaded (unsufficient priviledges?) */
const int      crc_verification_failed                    =266;    /* The crc verif of resource has failed; it is likely to be corrupted */
const int      critical_error                             =267;    /* Generic critical error (system crash imminent?) */
const int      data_error                                 =268;    /* Data read (from I/O ports) is corrupted (Generic error) */

const int      driver_is_not_valid                        =269;
const int      driver_not_found                           =270;
const int      drive_not_ready                            =271;    /* A floppy isn't responding. DOS/PC only? */

const int      duplicate_identifier_in_list               =272;    /* Warning: The object has 'uniqueness' flag set and there was an addition of an element already in the list */
const int      end_of_list                                =273;    /* Warning: The object can't forward anymore (you have reach the list's tail) */
const int      error_found_in_resource                    =274;
const int      error_in_driver                            =275;

const int      file_is_readonly                           =276;    /* Can't write to that file (either because it's readonly or because you lack access priviledges) */
const int      file_crc_failed                            =277;    /* File is corrupted */
const int      file_structure_is_altered                  =278;    /* File might be still readable, but has unexpected structure */
const int      incorrect_parameter                        =279;    /* One parameter was incorrect */
const int      internal_stack_error                       =280;    /* A stack-based object ran out of stack space or poped beyond stack bottom */

const int      invalid_file_format                        =281;    /* The file format is invalid for this operation */
const int      invalid_handle                             =282;    /* The handle is invalid (for this operation) */
const int      invalid_index_key                          =283;    /* Index is invalid */
const int      invalid_object_mode                        =284;    /* The object can't operate under this mode (e.g., wrong class level )*/
const int      invalid_object_size                        =285;    /* The size of the object does not match the expected size (e.g., wrong class level) */
const int      invalid_operation                          =286;    /* This operation is invalid */

const int      invalid_password                           =287;
const int      invalid_password_level                     =288;
const int      invalid_password_privilege                 =289;
const int      invalid_pointer_operation                  =290;
const int      invalid_task_handle                        =291;
const int      invalid_task_priority                      =292;
const int      invalid_window_coordinates                 =293;

const int      last_record                                =294;    /* Warning: End of List (or last database record) */
const int      last_resource                              =295;    /* Warning: Enf of Resource file (or list, or... ) */
const int      low_mem                                    =296;    /* Warning: Memory (of any type, depends on context) is running (dangerously) low */

const int      mem_alloc_error                            =297;    /* Could not allocate memory for this object */
const int      mem_dealloc_error                          =298;    /* Could not deallocate memory for this object (someone still points on it?) */
const int      nil_pointer_operation                      =299;    /* A NULL pointer was involved. */
const int      not_enough_disk_space                      =300;    /* There was not enough disk space to complete operation */
const int      not_enough_memory                          =301;    /* There was not enough memory to complete operation */

const int      object_allocation_error                    =302;    /* The object could not be allocated */
const int      object_class_mismatch                      =303;    /* The operation involves the wrong class types */
const int      object_data_destruction                    =304;    /* Warning: This operation (other than destroy and flush) has caused information destruction */
const int      object_dimension_mismatch                  =305;    /* Object of the same (super) classe mismatch (e.g., uncompatible matrices) */

const int      object_is_not_an_icon                      =306;    /* This object is not an Icon */

const int      object_not_initialized                     =307;    /* Warning: the object is empty and unclean */

const int      out_of_task_space                          =308;    /* Can't create another task */
const int      password_file_crc_failed                   =309;    /* Password file either corrupted or have been tempered */
const int      password_file_not_found                    =310;    /* Password file not found (deleted illegaly) */

const int      range_check_error                          =311;    /* The index is out of bounds */
const int      record_not_found                           =312;    /* The index points to an unexistant record */
const int      resource_index_altered                     =313;    /* The index is corrupted */
const int      resource_not_added                         =314;    /* Resource was'nt modified due to file access rights */
const int      resource_not_found                         =315;    /* Invalid resource name/description */
const int      resource_type_mismatch                     =316;    /* The resource exists but does not match the expected type */

const int      type_mismatch                              =317;    /* Types don't match for the operation */
const int      unexpected_eof                             =318;    /* The file ends before its expected ends (file corruption?) */
const int      unexpected_file_missing                    =319;    /* The file is missing but should still be there (file deleted by another process?) */

const int      insufficient_password_level                =320;
const int      window_not_on_top                          =321;
const int      dll_not_a_dll                              =322;
const int      dll_procedure_not_found                    =323;
const int      dll_cant_unlink                            =324;
const int      dll_cant_link                              =325;
const int      dll_invalid_dll_type                       =326;
const int      dll_not_found                              =325;

const int      structure_too_large                        =326;    /* The object is bigger than the active segment (PC real mode only) */
const int      list_is_empty                              =327;    /* Warning: The operation cannot be performed on an empty list */
const int      head_of_list                               =328;    /* You are at the head of the list and can't rewind further (result of a 'seek_previous') */

const int      node_no_smaller                            =329;    /* Warning: This is the smallest (internal) node */
const int      node_no_greater                            =330;    /* Warning: This is the largest (internal) node */
const int      node_is_leaf                               =331;    /* Warning: you have reached a leaf */

const int      no_free_channel                            =332;
const int      no_free_resource                           =333;

const int      hole_in_list                               =334;    /* A mislink has been found (critical error) */
const int      cannot_rewind                              =335;    /* Single-link lists can't rewind */
const int      invalid_pointer_declaration                =336;    /* The use of a NULL pointer, or a pointer on the wrong type of object */

const int      resource_not_removed                       =337;    /* Due to access rights */
const int      resource_bad_priviledge                    =338;    /* File is accessible but another process has a lock on it */
const int      resource_cannot_be_reorganized             =339;    /* Warning: either ressource packing has been already done or this type of resource can't be indexed and reorganised */

const int      not_a_password_file                        =340;    /* The specified file is not (even near) a valid password file */
const int      priviledge_fault                           =340;    /* The priviledges for the current user are unsufficient for this operation */
const int      unknown_user                               =341;    /* Who the hell are you? */

/* errors related to databases */

const int      database_cyclical_xreference               =342;
const int      database_missing_reference                 =343;
const int      database_invalid_data_type                 =344;
const int      database_invalid_field_reference           =345;
const int      database_invalid_index_reference           =346;
const int      database_no_index                          =347;
const int      database_referee_must_be_unique            =348;
const int      database_index_overflow                    =349;
const int      database_empty                             =350;
const int      database_field_uniqueness_violation        =360;
const int      database_no_selection_found                =361;
const int      database_too_many_fields                   =362;

/* errors related to files formats */

const int      not_a_supported_file_format                =363;    /* The file format has been correctly recognized but use an unsupported feature */
const int      file_may_be_crippled                       =364;    /* The bit stream is not what it should be (wrong file format or damage file) */

/* errors related to xms: uninteresting, except for DOS real mode. */

const int      xms_not_present                            =365;
const int      xms_badfunction                            =366;
const int      xms_vdiskdetected                          =367;
const int      xms_a20error                               =368;
const int      xms_drivererror                            =369;
const int      xms_unrecerror                             =370;
const int      xms_nohma                                  =371;
const int      xms_hmainuse                               =372;
const int      xms_hmaminerror                            =373;
const int      xms_hmanotalloc                            =374;
const int      xms_a20enabled                             =375;
const int      xms_nomoremem                              =376;
const int      xms_nomorehandles                          =377;
const int      xms_badhandle                              =378;
const int      xms_badsourceh                             =379;
const int      xms_badsourceo                             =380;
const int      xms_baddesth                               =381;
const int      xms_baddesto                               =382;
const int      xms_badlength                              =383;
const int      xms_badoverlap                             =384;
const int      xms_parityerror                            =385;
const int      xms_blknotlocked                           =386;
const int      xms_blklocked                              =387;
const int      xms_blklcoverflow                          =388;
const int      xms_lockfailed                             =389;
const int      xms_smallerumb                             =390;
const int      xms_noumb                                  =391;
const int      xms_badumbsegment                          =392;

/* Other errors related to objects */

const int      object_initialized_but_empty               =393;    /* Warning: object is clean but empty */
const int      object_save_failed                         =394;    
const int      object_duplication_failed                  =395;    /* most probably because the use of an uninitialized object on either side of the transaction */
const int      node_is_not_a_leaf                         =396;    /* Your have tried an operation on a non-leaf node that requires leaf-nodes. */
const int      invalid_mode                               =397;    /* This mode (parameter) is invalid */
const int      range_mismatch                             =398;    /* Bounds of interacting object don't match. I.e., incompatible matrices. */
const int      registry_new_entry                         =399;    /* Warning: The class name was not already in the registry */
const int      registry_existing_entry                    =400;    /* Warning: The class name was already in the registry */
const int      cant_go_down                               =401;    /* Warning: Can't go down one level (at that node) */
const int      cant_go_up                                 =402;    /* Warning: Can't go up one level (at that node) */
const int      mem_realloc_error                          =403;    /* memory block could not be resized */
const int      registry_unknown_class                     =404;





// --------------------- #include "utils.h"

/*
   some ugly (but usefull) defines and things.

 */


/* Defines, macros, etc. */


#define swap(a,b) {\
                   char c; \
                   c=a; a=b; c=b; \
                   } 

#define ifi(c,x,y)  ( (c) ? (x):(y) )

#define min(a,b) ((a)>(b) ? (b):(a))
#define max(a,b) ((a)>(b) ? (a):(b))


#define decnz(x)  if ((x)>0) x--
#define inctop(x,y) if ((x)<(y)) x++

#define iff(cond,a,b)  ((cond) ? (a):(b))

#define sqr(x)   ((x)*(x))
#define cube(x)  ((x)*(x)*(x))


/* functions giving information on the current machine */


/******************************/
int indien_int()
    {
     int   i = 0x12345678;
     char *p;


     p=(char *)&i;

     if (*p==0x12) 
          return 1; 
     else if (*p==0x78)
               return 0;
          else return 2;
    }

/******************************/
int indien_short()
    {
     short i = 0x1234;
     char *p;

     p=(char *)&i;

     if (*p==0x12) 
          return 1; 
     else if (*p==0x34)
               return 0;
          else return 2;
    }



// ------------------ #include "types.h"



/* 


   definitions of constants and common types 


*/




#define True  1
#define False 0

#define Yes   1
#define No    0

typedef unsigned char boolean;

typedef unsigned char      xlat_table[256];
typedef int                count_table[256];

typedef struct 
         { 
          unsigned char r,g,b; 
         } color;

typedef struct
         {
           unsigned char a,r,g,b;
         } alphacolor;

typedef struct 
         { 
          unsigned char b,g,r; 
         } r_color;

typedef struct
         {
          unsigned char b,g,r,a;
         } r_alphacolor;

typedef union  
         { unsigned char        b[4096];
           signed   char        s[4096];
           unsigned short int   w[2048];
           signed   short int   i[2048];
           unsigned int         l[1024];
           unsigned char       _8[32][32];
           unsigned short int _15[32][32];
           unsigned short int _16[32][32];
           r_color            _24[32][32];
           r_color              c[1365];
         } corpus;

typedef struct 
         {
          int length;
          unsigned char bytes_per_unit;
          unsigned char bits_per_unit;
          corpus corps;
         } bitstring;




// --------------- #include "oae.c"
/*

   This unit contains basic string maniplation
   for ISO8859-1.


   by Steven Pigeon


*/

const char downcase_table[256]= { (char)0x00,(char)0x01,
(char)0x02,(char)0x03,(char)0x04,(char)0x05,(char)0x06,(char)0x07,(char)0x08,(char)0x09,
(char)0x0a,(char)0x0b,(char)0x0c,(char)0x0d,(char)0x0e,(char)0x0f,(char)0x10,(char)0x11,
(char)0x12,(char)0x13,(char)0x14,(char)0x15,(char)0x16,(char)0x17,(char)0x18,(char)0x19,
(char)0x1a,(char)0x1b,(char)0x1c,(char)0x1d,(char)0x1e,(char)0x1f,(char)0x20,(char)0x21,
(char)0x22,(char)0x23,(char)0x24,(char)0x25,(char)0x26,(char)0x27,(char)0x28,(char)0x29,
(char)0x2a,(char)0x2b,(char)0x2c,(char)0x2d,(char)0x2e,(char)0x2f,(char)0x30,(char)0x31,
(char)0x32,(char)0x33,(char)0x34,(char)0x35,(char)0x36,(char)0x37,(char)0x38,(char)0x39,
(char)0x3a,(char)0x3b,(char)0x3c,(char)0x3d,(char)0x3e,(char)0x3f,(char)0x40,(char)0x61,
(char)0x62,(char)0x63,(char)0x64,(char)0x65,(char)0x66,(char)0x67,(char)0x68,(char)0x69,
(char)0x6a,(char)0x6b,(char)0x6c,(char)0x6d,(char)0x6e,(char)0x6f,(char)0x70,(char)0x71,
(char)0x72,(char)0x73,(char)0x74,(char)0x75,(char)0x76,(char)0x77,(char)0x78,(char)0x79,
(char)0x7a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,(char)0x60,(char)0x61,
(char)0x62,(char)0x63,(char)0x64,(char)0x65,(char)0x66,(char)0x67,(char)0x68,(char)0x69,
(char)0x6a,(char)0x6b,(char)0x6c,(char)0x6d,(char)0x6e,(char)0x6f,(char)0x70,(char)0x71,
(char)0x72,(char)0x73,(char)0x74,(char)0x75,(char)0x76,(char)0x77,(char)0x78,(char)0x79,
(char)0x7a,(char)0x7b,(char)0x7c,(char)0x7d,(char)0x7e,(char)0x7f,(char)0x80,(char)0x81,
(char)0x82,(char)0x83,(char)0x84,(char)0x85,(char)0x86,(char)0x87,(char)0x88,(char)0x89,
(char)0x8a,(char)0x8b,(char)0x8c,(char)0x8d,(char)0x8e,(char)0x8f,(char)0x90,(char)0x91,
(char)0x92,(char)0x93,(char)0x94,(char)0x95,(char)0x96,(char)0x97,(char)0x98,(char)0x99,
(char)0x9a,(char)0x9b,(char)0x9c,(char)0x9d,(char)0x9e,(char)0x9f,(char)0xa0,(char)0xa1,
(char)0xa2,(char)0xa3,(char)0xa4,(char)0xa5,(char)0xa6,(char)0xa7,(char)0xa8,(char)0xa9,
(char)0xaa,(char)0xab,(char)0xac,(char)0xad,(char)0xae,(char)0xaf,(char)0xb0,(char)0xb1,
(char)0xb2,(char)0xb3,(char)0xb4,(char)0xb5,(char)0xb6,(char)0xb7,(char)0xb8,(char)0xb9,
(char)0xba,(char)0xbb,(char)0xbc,(char)0xbd,(char)0xbe,(char)0xbf,(char)0xe0,(char)0xe1,
(char)0xe2,(char)0xe3,(char)0xe4,(char)0xe5,(char)0xe6,(char)0xe7,(char)0xe8,(char)0xe9,
(char)0xea,(char)0xeb,(char)0xec,(char)0xed,(char)0xee,(char)0xef,(char)0xf0,(char)0xf1,
(char)0xf2,(char)0xf3,(char)0xf4,(char)0xf5,(char)0xf6,(char)0xd7,(char)0xf8,(char)0xf9,
(char)0xfa,(char)0xfb,(char)0xfc,(char)0xfd,(char)0xfe,(char)0xdf,(char)0xe0,(char)0xe1,
(char)0xe2,(char)0xe3,(char)0xe4,(char)0xe5,(char)0xe6,(char)0xe7,(char)0xe8,(char)0xe9,
(char)0xea,(char)0xeb,(char)0xec,(char)0xed,(char)0xee,(char)0xef,(char)0xf0,(char)0xf1,
(char)0xf2,(char)0xf3,(char)0xf4,(char)0xf5,(char)0xf6,(char)0xf7,(char)0xf8,(char)0xf9,
(char)0xfa,(char)0xfb,(char)0xfc,(char)0xfd,(char)0xfe,(char)0xff };

const char upcase_table[256]= { (char)0x00,(char)0x01,
(char)0x02,(char)0x03,(char)0x04,(char)0x05,(char)0x06,(char)0x07,(char)0x08,(char)0x09,
(char)0x0a,(char)0x0b,(char)0x0c,(char)0x0d,(char)0x0e,(char)0x0f,(char)0x10,(char)0x11,
(char)0x12,(char)0x13,(char)0x14,(char)0x15,(char)0x16,(char)0x17,(char)0x18,(char)0x19,
(char)0x1a,(char)0x1b,(char)0x1c,(char)0x1d,(char)0x1e,(char)0x1f,(char)0x20,(char)0x21,
(char)0x22,(char)0x23,(char)0x24,(char)0x25,(char)0x26,(char)0x27,(char)0x28,(char)0x29,
(char)0x2a,(char)0x2b,(char)0x2c,(char)0x2d,(char)0x2e,(char)0x2f,(char)0x30,(char)0x31,
(char)0x32,(char)0x33,(char)0x34,(char)0x35,(char)0x36,(char)0x37,(char)0x38,(char)0x39,
(char)0x3a,(char)0x3b,(char)0x3c,(char)0x3d,(char)0x3e,(char)0x3f,(char)0x40,(char)0x41,
(char)0x42,(char)0x43,(char)0x44,(char)0x45,(char)0x46,(char)0x47,(char)0x48,(char)0x49,
(char)0x4a,(char)0x4b,(char)0x4c,(char)0x4d,(char)0x4e,(char)0x4f,(char)0x50,(char)0x51,
(char)0x52,(char)0x53,(char)0x54,(char)0x55,(char)0x56,(char)0x57,(char)0x58,(char)0x59,
(char)0x5a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,(char)0x60,(char)0x41,
(char)0x42,(char)0x43,(char)0x44,(char)0x45,(char)0x46,(char)0x47,(char)0x48,(char)0x49,
(char)0x4a,(char)0x4b,(char)0x4c,(char)0x4d,(char)0x4e,(char)0x4f,(char)0x50,(char)0x51,
(char)0x52,(char)0x53,(char)0x54,(char)0x55,(char)0x56,(char)0x57,(char)0x58,(char)0x59,
(char)0x5a,(char)0x7b,(char)0x7c,(char)0x7d,(char)0x7e,(char)0x7f,(char)0x80,(char)0x81,
(char)0x82,(char)0x83,(char)0x84,(char)0x85,(char)0x86,(char)0x87,(char)0x88,(char)0x89,
(char)0x8a,(char)0x8b,(char)0x8c,(char)0x8d,(char)0x8e,(char)0x8f,(char)0x90,(char)0x91,
(char)0x92,(char)0x93,(char)0x94,(char)0x95,(char)0x96,(char)0x97,(char)0x98,(char)0x99,
(char)0x9a,(char)0x9b,(char)0x9c,(char)0x9d,(char)0x9e,(char)0x9f,(char)0xa0,(char)0xa1,
(char)0xa2,(char)0xa3,(char)0xa4,(char)0xa5,(char)0xa6,(char)0xa7,(char)0xa8,(char)0xa9,
(char)0xaa,(char)0xab,(char)0xac,(char)0xad,(char)0xae,(char)0xaf,(char)0xb0,(char)0xb1,
(char)0xb2,(char)0xb3,(char)0xb4,(char)0xb5,(char)0xb6,(char)0xb7,(char)0xb8,(char)0xb9,
(char)0xba,(char)0xbb,(char)0xbc,(char)0xbd,(char)0xbe,(char)0xbf,(char)0xc0,(char)0xc1,
(char)0xc2,(char)0xc3,(char)0xc4,(char)0xc5,(char)0xc6,(char)0xc7,(char)0xc8,(char)0xc9,
(char)0xca,(char)0xcb,(char)0xcc,(char)0xcd,(char)0xce,(char)0xcf,(char)0xd0,(char)0xd1,
(char)0xd2,(char)0xd3,(char)0xd4,(char)0xd5,(char)0xd6,(char)0xd7,(char)0xd8,(char)0xd9,
(char)0xda,(char)0xdb,(char)0xdc,(char)0xdd,(char)0xde,(char)0xdf,(char)0xc0,(char)0xc1,
(char)0xc2,(char)0xc3,(char)0xc4,(char)0xc5,(char)0xc6,(char)0xc7,(char)0xc8,(char)0xc9,
(char)0xca,(char)0xcb,(char)0xcc,(char)0xcd,(char)0xce,(char)0xcf,(char)0xd0,(char)0xd1,
(char)0xd2,(char)0xd3,(char)0xd4,(char)0xd5,(char)0xd6,(char)0xf7,(char)0xd8,(char)0xd9,
(char)0xda,(char)0xdb,(char)0xdc,(char)0xdd,(char)0xde,(char)0xff };

const char translate_table[256]= { (char)0x00,(char)0x01,
(char)0x02,(char)0x03,(char)0x04,(char)0x05,(char)0x06,(char)0x07,(char)0x08,(char)0x09,
(char)0x0a,(char)0x0b,(char)0x0c,(char)0x0d,(char)0x0e,(char)0x0f,(char)0x10,(char)0x11,
(char)0x12,(char)0x13,(char)0x14,(char)0x15,(char)0x16,(char)0x17,(char)0x18,(char)0x19,
(char)0x1a,(char)0x1b,(char)0x1c,(char)0x1d,(char)0x1e,(char)0x1f,(char)0x20,(char)0x21,
(char)0x22,(char)0x23,(char)0x24,(char)0x25,(char)0x26,(char)0x27,(char)0x28,(char)0x29,
(char)0x2a,(char)0x2b,(char)0x2c,(char)0x2d,(char)0x2e,(char)0x2f,(char)0x30,(char)0x31,
(char)0x32,(char)0x33,(char)0x34,(char)0x35,(char)0x36,(char)0x37,(char)0x38,(char)0x39,
(char)0x3a,(char)0x3b,(char)0x3c,(char)0x3d,(char)0x3e,(char)0x3f,(char)0x40,(char)0x61,
(char)0x62,(char)0x63,(char)0x64,(char)0x65,(char)0x66,(char)0x67,(char)0x68,(char)0x69,
(char)0x6a,(char)0x6b,(char)0x6c,(char)0x6d,(char)0x6e,(char)0x6f,(char)0x70,(char)0x71,
(char)0x72,(char)0x73,(char)0x74,(char)0x75,(char)0x76,(char)0x77,(char)0x78,(char)0x79,
(char)0x7a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,(char)0x60,(char)0x61,
(char)0x62,(char)0x63,(char)0x64,(char)0x65,(char)0x66,(char)0x67,(char)0x68,(char)0x69,
(char)0x6a,(char)0x6b,(char)0x6c,(char)0x6d,(char)0x6e,(char)0x6f,(char)0x70,(char)0x71,
(char)0x72,(char)0x73,(char)0x74,(char)0x75,(char)0x76,(char)0x77,(char)0x78,(char)0x79,
(char)0x7a,(char)0x7b,(char)0x7c,(char)0x7d,(char)0x7e,(char)0x7f,(char)0x80,(char)0x81,
(char)0x82,(char)0x83,(char)0x84,(char)0x85,(char)0x86,(char)0x87,(char)0x88,(char)0x89,
(char)0x8a,(char)0x8b,(char)0x8c,(char)0x8d,(char)0x8e,(char)0x8f,(char)0x90,(char)0x91,
(char)0x92,(char)0x93,(char)0x94,(char)0x95,(char)0x96,(char)0x97,(char)0x98,(char)0x99,
(char)0x9a,(char)0x9b,(char)0x9c,(char)0x9d,(char)0x9e,(char)0x9f,(char)0xa0,(char)0xa1,
(char)0xa2,(char)0xa3,(char)0xa4,(char)0xa5,(char)0xa6,(char)0xa7,(char)0xa8,(char)0xa9,
(char)0xaa,(char)0xab,(char)0xac,(char)0xad,(char)0xae,(char)0xaf,(char)0xb0,(char)0xb1,
(char)0xb2,(char)0xb3,(char)0xb4,(char)0xb5,(char)0xb6,(char)0xb7,(char)0xb8,(char)0xb9,
(char)0xba,(char)0xbb,(char)0xbc,(char)0xbd,(char)0xbe,(char)0xbf,(char)0x61,(char)0x61,
(char)0x61,(char)0x61,(char)0x61,(char)0x61,(char)0xe6,(char)0x63,(char)0x65,(char)0x65,
(char)0x65,(char)0x65,(char)0x69,(char)0x69,(char)0x69,(char)0x69,(char)0xf0,(char)0x6e,
(char)0x6f,(char)0x6f,(char)0x6f,(char)0x6f,(char)0x6f,(char)0xd7,(char)0x6f,(char)0x75,
(char)0x75,(char)0x75,(char)0x75,(char)0x79,(char)0xfe,(char)0xdf,(char)0x61,(char)0x61,
(char)0x61,(char)0x61,(char)0x61,(char)0x61,(char)0xe6,(char)0x63,(char)0x65,(char)0x65,
(char)0x65,(char)0x65,(char)0x69,(char)0x69,(char)0x69,(char)0x69,(char)0xf0,(char)0x6e,
(char)0x6f,(char)0x6f,(char)0x6f,(char)0x6f,(char)0x6f,(char)0xf7,(char)0x6f,(char)0x75,
(char)0x75,(char)0x75,(char)0x75,(char)0x79,(char)0xfe,(char)0xff };

/* oae: a€‚ß„ƒ…†bc‡deˆŠ‰‹fghiŒŽjklmnoøŸ÷pqrstu·“”’vwxy²z */

const char oae_table[256]= { (char)0x00,(char)0x01,
(char)0x02,(char)0x03,(char)0x04,(char)0x05,(char)0x06,(char)0x07,(char)0x08,(char)0x09,
(char)0x0a,(char)0x0b,(char)0x0c,(char)0x0d,(char)0x0e,(char)0x0f,(char)0x10,(char)0x11,
(char)0x12,(char)0x13,(char)0x14,(char)0x15,(char)0x16,(char)0x17,(char)0x18,(char)0x19,
(char)0x1a,(char)0x1b,(char)0x1c,(char)0x1d,(char)0x1e,(char)0x1f,(char)0x20,(char)0x21,
(char)0x22,(char)0x23,(char)0x24,(char)0x25,(char)0x26,(char)0x27,(char)0x28,(char)0x29,
(char)0x2a,(char)0x2b,(char)0x2c,(char)0x2d,(char)0x2e,(char)0x2f,(char)0x30,(char)0x31,
(char)0x32,(char)0x33,(char)0x34,(char)0x35,(char)0x36,(char)0x37,(char)0x38,(char)0x39,
(char)0x3a,(char)0x3b,(char)0x3c,(char)0x3d,(char)0x3e,(char)0x3f,(char)0x40,(char)0x41,
(char)0x49,(char)0x4a,(char)0x4c,(char)0x4d,(char)0x52,(char)0x53,(char)0x54,(char)0x55,
(char)0x5a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,(char)0x65,(char)0x66,
(char)0x67,(char)0x68,(char)0x69,(char)0x6a,(char)0x6f,(char)0x70,(char)0x71,(char)0x72,
(char)0x74,(char)0x75,(char)0x76,(char)0x77,(char)0x78,(char)0x79,(char)0x7a,(char)0x41,
(char)0x49,(char)0x4a,(char)0x4c,(char)0x4d,(char)0x52,(char)0x53,(char)0x54,(char)0x55,
(char)0x5a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,(char)0x65,(char)0x66,
(char)0x67,(char)0x68,(char)0x69,(char)0x6a,(char)0x6f,(char)0x70,(char)0x71,(char)0x72,
(char)0x74,(char)0x7b,(char)0x7c,(char)0x7d,(char)0x7e,(char)0x7f,(char)0x80,(char)0x81,
(char)0x82,(char)0x83,(char)0x84,(char)0x85,(char)0x86,(char)0x87,(char)0x88,(char)0x89,
(char)0x8a,(char)0x8b,(char)0x8c,(char)0x8d,(char)0x8e,(char)0x8f,(char)0x90,(char)0x91,
(char)0x92,(char)0x93,(char)0x94,(char)0x95,(char)0x96,(char)0x97,(char)0x98,(char)0x99,
(char)0x9a,(char)0x9b,(char)0x9c,(char)0x9d,(char)0x9e,(char)0x9f,(char)0xa0,(char)0xa1,
(char)0xa2,(char)0xa3,(char)0xa4,(char)0xa5,(char)0xa6,(char)0xa7,(char)0xa8,(char)0xa9,
(char)0xaa,(char)0xab,(char)0xac,(char)0xad,(char)0xae,(char)0xaf,(char)0xb0,(char)0xb1,
(char)0xb2,(char)0xb3,(char)0xb4,(char)0xb5,(char)0xb6,(char)0xb7,(char)0xb8,(char)0xb9,
(char)0xba,(char)0xbb,(char)0xbc,(char)0xbd,(char)0xbe,(char)0xbf,(char)0x42,(char)0x44,
(char)0x43,(char)0x46,(char)0x45,(char)0x47,(char)0x48,(char)0x4b,(char)0x4e,(char)0x50,
(char)0x4f,(char)0x51,(char)0x56,(char)0x58,(char)0x57,(char)0x59,(char)0xc0,(char)0xc1,
(char)0x60,(char)0x62,(char)0x61,(char)0x64,(char)0x63,(char)0xc2,(char)0xc3,(char)0x6b,
(char)0x6d,(char)0x6c,(char)0x6e,(char)0x73,(char)0xc4,(char)0xc5,(char)0x42,(char)0x44,
(char)0x43,(char)0x46,(char)0x45,(char)0x47,(char)0x48,(char)0x4b,(char)0x4e,(char)0x50,
(char)0x4f,(char)0x51,(char)0x56,(char)0x58,(char)0x57,(char)0x59,(char)0xc6,(char)0xc7,
(char)0x60,(char)0x62,(char)0x61,(char)0x64,(char)0x63,(char)0xc8,(char)0xc9,(char)0x6b,
(char)0x6d,(char)0x6c,(char)0x6e,(char)0x73,(char)0xca,(char)0xcb };





void strip(char to_strip, char *s)
 {
  int i=0;
  int j=0;

  while (s[i])
   if (s[i]!=to_strip)
        s[j++]=s[i++];
   else i++;
   s[j]=0;
 }

char upcase(char a)
 {
  return upcase_table[(unsigned char)a];
 }

char downcase(char a)
 {
  return downcase_table[(unsigned char)a];
 }

char translate(char a)
 {
  return translate_table[(unsigned char)a];
 }

void translate_string(register char *s)
 {
  register unsigned char c;
  while (c=(unsigned char)*s)
    *s++=translate_table[c];
 }

void upcase_string(register char *s)
 {
  register unsigned char c;
  while (c=(unsigned char)*s)
    *s++=upcase_table[c];
 }

void downcase_string(register char *s)
 {
  register unsigned char c;
  while (c=(unsigned char)*s)
    *s++=downcase_table[c];
 }

void oae_string(register char *s)
 {
  register unsigned char c;
  while (c=(unsigned char)*s)
    *s++=oae_table[c];
 }

int oae_compare(const char *a, const char *b)
 {
  char a2[256];
  char b2[256];
  int i;

  strcpy(a2,a);
  strcpy(b2,b);

  strip('-',a2);
  strip('-',b2);

  translate_string(a2);
  translate_string(b2);

  i=strcmp(a2,b2);

  if (i)
       return i;
  else 
     {
      strcpy(a2,a);
      strcpy(b2,b);

      strip('-',a2);
      strip('-',b2);

      oae_string(a2);
      oae_string(b2);

      i=strcmp(a2,b2);

      if (i==0)
           return strcmp(a,b);
      else return i;    
     }
 }






// -----------------#include "listes.cpp"

/*


  Ce module contient un certain nombre de listes, traduits a partir 
  des listes de VX (en pascal).


*/


/*** definitions des objets contenus dans le module ***********************************/

typedef struct abstract_list_node
         {
          abstract_list_node *previous;
          abstract_list_node *next;
          void               *contenu;
          int                size;
         } abstract_list_node;


typedef struct list_node
         {
           list_node *previous;
           list_node *next;
           char      *contenu;
           int       size;
           int       count;
         } list_node;


typedef struct tree_node
         {
           tree_node *left;
           tree_node *right;
           tree_node *parent;
           tree_node *previous;
           tree_node *next;
           char      *contenu;
           int       size;
           int       count;
         } tree_node;

/* ****************************************************************************************************************************

    La classe string_list permet de maintenir une liste de char * (et non pas de strings qui
    sont definies dans les libs de base de C++). On peut faire des ajouts en ordre, en desordre,
    et quick_sorter la liste. Il est aussi possible de maintenir la liste en OAE (ordre alpha-
    betique etendue) pour avoir un ordre lexicographique correct (plutot que pseudo-lexico-ascii).

   **************************************************************************************************************************** */


class string_list 
      {
       
       private:  int         state;                 /* etat de l'objet */
                 list_node  *head;                  
                 list_node  *tail;
                 list_node  *current;
                 int         combien;               /* combien de chaines */
                 void        quick_sort_ascendant(list_node *tete,list_node *queue,int lo,int hi);
                 void        quick_sort_descendant(list_node *tete,list_node *queue,int lo,int hi);
                 void        insere_avant(const char *s);
                 void        insere_apres(const char *s);
                 void        exchange(list_node *a, list_node *b);
                 int         compare(const char *a, const char *b);

       public:   int         erreur;                /* la derniere erreur survenue.          */
                 int         uniqueness;            /* determine si les chaines sont uniques */
                 int         oae;                   /* ordre alphabetique etendue?           */
                 

                 void        flush();

                 void        reset();               /* retourne au premier element       */
                 int         seek(const char *s);   /* deplace vers l'element le plus pres de s (retourne vrai si s est dans la liste */
                 int         seek_next();           /* deplace vers l'element suivant    */
                 int         seek_previous();       /* deplace vers l'element precedent  */
                 void        seek_last();           /* deplace vers le dernier element   */

                 void        add(const char *s);    /* ajoute au debut ou a la fin                */
                 void        append(const char *s); /* Ajoute a la queue de la liste la string s  */
                 void        touch();               /* incremente le compteur a la position cour. */
                 void        replace(const char *s);     /* remplace la chaine presente par s     */                 
                 void        put(const char *s);         /* insere en preordre  */
                 void        reverse_put(const char *s); /* insere en postordre */
                 int         remove();              /* detruit l'item courant.                   */
                 char       *get_ref();             /* retourne un pointeur a l'item             */
                 list_node  *get_node_ref();        /* retourne 'current'                        */
                 int         get(char *s);          /* copie vers s, sans allocation             */
                 int         get_count();           /* donne le nombre de fois que la chaine courante a ete vue */
                 int         sort(int sens);        /* trie la liste par quicksort (0=ascendant, 1=descendant)              */
                 int         count();               /* combien d'elements                        */

                 string_list();
                 ~string_list();
      };

class ticket
         {
           void *pointeur;    /* pointeur a quelque chose (mais on sait pas quoi) */
           int   state;       /* etat interne du ticket (qui peut etre n'importe quoi). */

           protected: ticket();
                      ~ticket();
         };


/* ****************************************************************************************************************************

     This is a general node for a list of list

   **************************************************************************************************************************** */

class list_of_list_node
       { 

        public: boolean  object;               /* 0=points to a lists, 1=points to some object */
                int      size;                 /* size of that object. If size==0, then it's only a reference */
                void    *contenu;              /* points to whatever object you like */

                list_of_list_node *previous;   /* points to previous object on same level. if null, previous should be ->parent->next */
                list_of_list_node *next;       /* points to next object on same level. If null, next should be ->parent->next */
                list_of_list_node *parent;     /* points to the up level */
                list_of_list_node *kid;        /* if object==0 then points to a sublist */

                list_of_list_node();
                ~list_of_list_node();

       };

/* ****************************************************************************************************************************

   The ref list is a very simple linked list. You only have a list of 
   pointers to objects (and you must make sure what kind of object it 
   is because ref_list does not know it).

   **************************************************************************************************************************** */

class ref_list_node
      {
       private: 
       
       public:   ref_list_node *next;
                 void          *ref;

                 ref_list_node();
                 ~ref_list_node();
      };


class ref_list
      {

       private:  ref_list_node *head;
                 ref_list_node *current;
                 

       public:   int combien;

                 void append(const void *something);   /* appends a ref to the list */
                 void reset();                        /* sets current to head       */

                 int  seek_next();                  /* sets current to next item in list */
                 void *get_ref();                  /* tells what object is linked to this node */

                 int  get_count();               /* tells how many object there is in the list */

                 void flush();                 /* explicit destructor of ref_list */

                 ref_list();
                 ~ref_list();

      };
 


/* **************************************************************************************************************************** 

   The class list_of_list is a general object for implementing lists of ... lists of
   objects (a la lisp). 

   The nested lists are managed with "gris" nodes. "Gris" nodes are nodes that do not contains object. 

   For example : ( ( ( a ) b c ) d )

   is created by

       add();                  // creates a sublist; and set current into that sublist 
       add();                 // creates a sublist; and set current into that sublist 
       add();                // creates a sublist; and set current into that sublist 
       add(2,"a");          // adds a non-list element string "a"
       seek_up();          // exits this sublist, goes one level up
       append(2,"b");     // adds a non-list element string "b"
       append(2,"c");    // adds a non-list element string "c"
       seek_up();       // exits this sublist, goes one level up
       append(2,"d");  // adds a non-list element string "d"
   

   **************************************************************************************************************************** */


class list_of_list
      {

       private:  int                state;                 /* state: 0 = clean, 1 = non empty */
                 int                combien;               /* numbers of node */
                 list_of_list_node *head;                  /* first node */
                 list_of_list_node *current;               /* current node */

                 void               recursive_remove(list_of_list_node *p);  /* recursively destroys a sub-list */
                 void               remove_node(list_of_list_node *p);
                 
       public:   int                erreur;                /* la derniere erreur survenue. */
                 
                 void               flush();               /* detruit la liste */

                 void               reset();               /* return to the root, first element */
                 void               seek_first();          /* goes to the first element, on this level */
                 void               seek_last();           /* goes to the last element, on this level */
                 int                seek_next();           /* goes to the next element, if possible */
                 int                seek_previous();       /* goes to the previous element, if possible */
                 int                seek_down();           /* goes down a level, if possible */
                 int                seek_up();             /* goes up a level, if possible */
                 


                 void               add();                             /* adds a list (a "gris" node) at current node (preorder) */
                 void               add(int size, void *something);    /* adds something of sizeof(size) at current node (preorder) (a copy is made) */
                 void               add(void *something);              /* adds a reference at the current node (preorder) (no copy is made)*/
                 void               append();                          /* appends a list (a "gris" node) at this level */
                 void               append(int size, void *something); /* appends an object at this level (a copy is made) */
                 void               append(void *something);           /* appends a reference at the current node (preorder) (no copy is made) */
                 void               remove();                          /* destroys current item (list or object!) */

                 int                size();                /* 0 = list, otherwise, size of the object */
                 int                list();                /* 1 = if this node is a list, 0 = some other object */

                 void              *get_ref();             /* returns a pointer to the object */
                 list_of_list_node *get_node_ref();        /* retourne 'current' */
                 int                get(void *s);          /* copie vers s, sans allocation (i.e., s pointe deja a un objet) */
                 int                count();               /* combien d'elements                                          */

                 list_of_list();
                 ~list_of_list();
      };


/* ****************************************************************************************************************************

   La classe string_tree maintient une liste TOUJOURS triee... alors si on l'utilise
   comme une string_list, on risque d'avoir quelques problemes lorsqu'on s'attend a
   pourvoir maintenir un ordre quelconque dans la liste.

   **************************************************************************************************************************** */

class string_tree
       {
        private: tree_node *root;
                 tree_node *head;
                 tree_node *tail;

                 int        combien;
                 int        state;

                 void       overthread(tree_node *p);
                 void       movefrom(tree_node *p);
                 int        compare(const char *a, const char *b);
 
        public:  int        erreur;
                 int        uniqueness;
                 int        oae;
                 tree_node *current;

                 void       flush();
                 
                 void        reset();
                 int         seek(const char *s);
                 int         seek_next();
                 int         seek_previous();
                 int         seek_first();
                 int         seek_last();
                 void        add(const char *s);
                 void        put(const char *s);
                 void        touch();
                 void        replace(const char *s);
                 void        remove();
                 char       *get_ref();
                 tree_node  *get_node_ref();
                 int         get(char *s);
                 int         get_count();
                 int         count();

                 string_tree();
                 ~string_tree();
       };


/* ****************************************************************************************************************************

    Node for a trie

   **************************************************************************************************************************** */ 

typedef struct
        {
          unsigned char occupant;   /* Le contenu de cette case */
          int           parent;     /* Le parent (precedant)    */
          int           frere;      /* Le (prochain) frere      */
          int           fils;       /* Le (premier) fils        */
          int           count;      /* Combien de mots passant par ce point */
          int           combien;    /* Combien de fils? */
        } noeud_de_s_treille;

typedef noeud_de_s_treille *  pTreille_tableau;


/* **************************************************************************************************************************** 

   La s_treille est une variation (au niveau de l'implantation) de la treille traditionnelle.
   Realise un arbre de degre variable (et possiblement tres grand). Les methodes de deletion
   des noeuds ne correspondent pas a celles des arbres generaux. On ne coupe que les feuilles
   les plus jeunes. 

   **************************************************************************************************************************** */

class s_treille
        {
         private:

         public:    int sizeof_treille;
                    int combien;
                    pTreille_tableau c;

                    int pointeur;

                    void init(int size, int maxx);         /* Size=combien de noeuds au total, maxx=combien de symboles */
                    void flush();
                    void reset();                          /* retour a la racine */
                    int  seek(unsigned char i);            /* A cet etage, y a-t-il un frere i ? */
                    int  descend();                        /* S'il y a un fils, on y va */
                    int  add_frere(unsigned char i);
                    int  add_fils(unsigned char i);
                    int  libere(int nombre);               /* Libere un certain nombre de noeuds */
                    int  conserve(int nombre);             /* Conserve un certain nombre de noeuds, libere les autres */

                    void touch_word();                     /* +1 sur tout les noeuds menant du noeud courant a la racine */
                    void touch_node();                     /* +1 sur le noeud courant seulement */

                    void extract(unsigned char *s, int position); /* Extrait les caracteres de la racine au noeud courant */

                    // Info 

                    int  count();
                    int  erreur;
                    int  full();

                    // constructeur/destructeur par defaut 

                    s_treille();
                    ~s_treille();
        };


/* ****************************************************************************************************************************

   The large array class is exactly that... an array.
   It saves lots of memory if it's going to be a sparse array, like hash tables, for instance :-) .

   **************************************************************************************************************************** */

class large_array
       {
        private:    void  **table;
                    int   _size,_combien;
                    int   state;
                    
        public:     int   erreur;
                    
                    void  init(int combien, int size);

                    void  put(int n, void *p);
                    void  get(int n, void *p);
                    void* ref(int n);

                    void  flush();

                    /* constructeurs & destructeurs par defaut */

                    large_array();
                    ~large_array();
       };


/* ****************************************************************************************************************************

   The dynamic_array array classe minimise or maximise its size according to the
   following:

      if the array is full, then it double its size.
      
      if you know the array is 3/4 empty, you should halve it.


   Ok, ok, I know, templates aren't that great, but to use something else would be... well...

   **************************************************************************************************************************** */

template <class typeT> class dynamic_array
       {
        private: int    state;
                 typeT *array;
                 int    size;
                 int    maxindex;

        public:  int erreur;
        
                 void reset();                  /* clears erreur */

                 int double_size();             /* 0 = did not, 1 = did */
                 int halve_size();              /* 0 = did not, 1 = did */
                 int inbound(int i);            /* 1 = i<size; 0 otherwise */

                 typeT & operator[](int index);

                 int set_size(int max);         /* 0 = did,  1 = did not */
                 int get_size();                /* sizeof(array) in number of element of type typeT */    

                 void flush();                  /* destructor */

                 dynamic_array(int max = 256 );        /* creates the board with initial size max (defauts to 256) */
                 ~dynamic_array();
       };

/* **************************************************************************************************************************** */

/***Defs de template <class typeT> class dynamic_array *************************************************/
           

/**********************************************************************/
template <class typeT> void dynamic_array<typeT>::reset()
 { 
  erreur=none;
 }
           
/**********************************************************************/
template <class typeT> int dynamic_array<typeT>::double_size()
 {
  if (set_size(size*2))
     {
      erreur=none;
      return 1;
     }
  else return 0;
 }


/**********************************************************************/
template <class typeT> int dynamic_array<typeT>::halve_size()
 {
  if (set_size(size/2))
     {
      erreur=none;
      return 1;
     }
  else return 0;
 }

/**********************************************************************/
template <class typeT> typeT & dynamic_array<typeT>::operator[](int index)
 {
  if (index<size)
     {
      maxindex=max(index,maxindex);
      return array[index];
     }
  else {
        if (set_size(index*2))
             return array[index];
        else return array[0];      /* Ach! Grož malheur! */
       }
 }

/**********************************************************************/
template <class typeT> int dynamic_array<typeT>::inbound(int i)
 {
  return (i<size);
 }


/**********************************************************************/
template <class typeT> int dynamic_array<typeT>::set_size(int max)
 {
  if (state)
     {
      typeT *z;

      z=(typeT *)realloc(array,(size_t)(max*sizeof(typeT)));

      if (z)
         {
          array=z;
          size=max;
          maxindex=min(max,maxindex);
          erreur=none;
          return 1;
         }
      else {
            erreur=mem_realloc_error;
            return 0;
           }
     }
  else {
        array=(typeT *)malloc((size_t)(max*sizeof(typeT)));

        if (array)
           { 
            size=max;
            maxindex=max;
            erreur=none;
            return 1;
           }
        else {
              erreur=object_initialized_but_empty;
              return 0;
             }
       }
 }

/**********************************************************************/
template <class typeT> int dynamic_array<typeT>::get_size()
 {
  return size;
 }


/**********************************************************************/
template <class typeT> dynamic_array<typeT>::dynamic_array(int max)
 {
  maxindex=0;
  size=max;
  array=(typeT *)malloc(max * sizeof(typeT));
  if (array)
      {
       state=1;
       erreur=none;
      }
  else {
        state=0;
        erreur=not_enough_memory;
       }
 }

/**********************************************************************/
template <class typeT> void dynamic_array<typeT>::flush()
 {
  if (state) free(array);
  size=0;
  maxindex=0;
  state=0;
 }

/**********************************************************************/
template <class typeT> dynamic_array<typeT>::~dynamic_array()
 {
  flush();
 }


/* **************************************************************************************************************************** */

/***Defs de ref_list_node ******************************************************************************/


/******************************/
ref_list_node::ref_list_node()
 {
  next=NULL;
  ref=NULL;
 }

/******************************/
ref_list_node::~ref_list_node()
 {
  /* poof */
 }



/***Defs de ref_list ***********************************************************************************/

/******************************/
void ref_list::append(const void *something)
 {
  if (combien)
     {
      while (seek_next());

      current->next = new ref_list_node;
      current=current->next;
      current->ref= (void *) something;

      combien++;
     }
  else {
        head = new ref_list_node;
        head->ref = (void *)something;
        current=head;
        combien=1;
       }
 }

/******************************/
void ref_list::reset()
 {
  current=head;
 }

/******************************/
int  ref_list::seek_next()
 {
  if (current->next) 
     {
      current = current->next;
      return 1;
     }
  else return 0;
 }

/******************************/
void *ref_list::get_ref()
 {
  return current->ref;
 }

/******************************/
int  ref_list::get_count()
 {
  return combien;
 }

/******************************/
void ref_list::flush()
 {
  ref_list_node *t;

  if (combien)
     {
      reset();
      do
       {
        t=current->next;
        delete current;
        current=t;
       }
      while (t);
      combien=0;
      head=NULL;
      current=NULL;
     };
 }

/******************************/
ref_list::ref_list()
 {
  combien=0;
  head=NULL;
  current=NULL;
 }

/******************************/
ref_list::~ref_list()
 {
  flush();
 }


/*** Definitions for list_of_list_node ************************************************/


/*************************************/
list_of_list_node::list_of_list_node()
 {
  next     = NULL;
  previous = NULL;
  parent   = NULL;
  kid      = NULL;
  contenu  = NULL;

  size     = 0;
  object   = 0;
 }

/*************************************/
list_of_list_node::~list_of_list_node()
 {
   /* poof */
 }


/* **************************************************************************************************************************** */

/*** Definitions for list_of_list *****************************************************/

/*****************************/
void list_of_list::flush()
 {
  if (state) recursive_remove(head);
  head=NULL;
  combien=0;
  state=0;
  erreur=object_initialized_but_empty;
 }

/*****************************/
void list_of_list::reset()
 {
  current=head;
  erreur=none;
 }

/*****************************/
void list_of_list::seek_first()
 {
  if (current)
      while (seek_previous());
  else erreur=list_is_empty;
 }

/*****************************/
void list_of_list::seek_last()
 { 
  if (current)
      while (seek_next());
  else erreur=list_is_empty;
 }

/*****************************/
int  list_of_list::seek_next()
 { 
  if (current)
      if (current->next)
          {
           current=current->next;
           erreur=none;
           return 1;
          }
      else {
            erreur=end_of_list;
            return 0;
           }
  else {
        erreur=list_is_empty;    
        return 0;
       }
 }

/*****************************/
int  list_of_list::seek_previous()
 {
  if (current) 
      if (current->previous)
          {
           current=current->previous;
           erreur=none;
           return 1;
          }
      else {
            erreur=head_of_list;
            return 0;
           }
  else {
        erreur=list_is_empty;
        return 0;
       }
 }

/*****************************/
int  list_of_list::seek_down()
 {
  if (current)
      if (current->kid)
          {
           current=current->kid;
           erreur=none;
           return 1;
          }
      else {
            erreur=cant_go_down;
            return 0;
           }
   else {
         erreur=list_is_empty;
         return 0;
        }

 }

/*****************************/
int  list_of_list::seek_up()
 {
  if (current)
      if (current->parent)
           {
            current=current->parent;
            erreur=none;
            return 1;
           }
      else {
            erreur=cant_go_up;
            return 0;
           }
  else {
        erreur=list_is_empty;
        return 0;
       }
 }

/*****************************/
void list_of_list::add()
 {
  list_of_list_node *p;

  p = new list_of_list_node;

  if (combien)
      {
       if (current->object==0) /* "gris" */
          {
           current->kid=p;
           p->parent=current;
           current=p;
          }
       else {
             if (current->previous) current->previous->next = p;
             p->previous=current->previous;
             current->previous=p;
             p->next=current;
             p->parent=current->parent;
             if (current==head) head=p;
             current=p;
            }
      } 
  else {
        head=p;
        current=p;
       }

  state=1;
  combien++;
  erreur=none;
 }

/*****************************/
void list_of_list::add(int size, void *something)
 {
  list_of_list_node *p;

  p = new list_of_list_node;
  p->object=1;
  p->size=size;
  p->contenu = malloc(size);
  memcpy(p->contenu,something,size);

  if (combien)
      { 
       if (current->object==0)  /* "gris" */
          {
           current->kid=p;
           p->parent=current;
           current=p;
          }
       else {
             if (current->previous) current->previous->next = p;
             p->previous=current->previous;
             p->next=current;
             p->parent=current->parent;
             current->previous=p;
             if (current==head) head=p;
             current=p;
            }
      } 
  else {
        head=p;
        current=p;
       }

  state=1;
  combien++;
  erreur=none;
 }


/*****************************/
void list_of_list::add(void *something)
 {
  list_of_list_node *p;

  p = new list_of_list_node;
  p->object=1;
  p->size=0;
  p->contenu = something;

  if (combien)
      { 
       if (current->object==0)  /* "gris" */
          {
           current->kid=p;
           p->parent=current;
           current=p;
          }
       else {
             if (current->previous) current->previous->next = p;
             p->previous=current->previous;
             p->next=current;
             p->parent=current->parent;
             current->previous=p;
             if (current==head) head=p;
             current=p;
            }
      } 
  else {
        head=p;
        current=p;
       }

  state=1;
  combien++;
  erreur=none;
 }

/*****************************/
void list_of_list::append()
 {
  list_of_list_node *p;

  p = new list_of_list_node;
  
  seek_last();

  if (combien)
      {
       current->next=p;
       p->parent=current->parent;
       p->previous=current;
       current=p;
      } 
  else {
        head=p;
        current=p;
       }

  state=1;
  combien++;
  erreur=none;
 }

/*****************************/
void list_of_list::append(int size, void *something)
 {
  list_of_list_node *p;

  p = new list_of_list_node;
  p->object=1;
  p->size=size;
  p->contenu = malloc(size);
  memcpy(p->contenu,something,size);

  seek_last();
  
  if (combien)
      {
       current->next=p;
       p->parent=current->parent;
       p->previous=current;
       current=p;
      } 
  else {
        head=p;
        current=p;
       }

  state=1;
  combien++;
  erreur=none;
 }

/*****************************/
void list_of_list::append(void *something)
 {
  list_of_list_node *p;

  p = new list_of_list_node;
  p->object=1;
  p->size=0;
  p->contenu = something;
  
  seek_last();
  
  if (combien)
      {
       current->next=p;
       p->parent=current->parent;
       p->previous=current;
       current=p;
      } 
  else {
        head=p;
        current=p;
       }

  state=1;
  combien++;
  erreur=none;
 }

/*****************************/
void  list_of_list::remove_node(list_of_list_node *p)
 {
  if (head==p) head=p->next;

  if (p->next) p->next->previous=p->previous;
  if (p->previous) p->previous->next=p->next;

  if (p->object)
       {
        if (p->size)
            free(p->contenu);
        p->contenu=NULL;
       }
  else if (p->kid) recursive_remove(p->kid);

  combien--;
 }

/*****************************/
void  list_of_list::recursive_remove(list_of_list_node *p)
 { 
  list_of_list_node *n;
  while (p->next)
   { 
    n=p->next;
    remove_node(p);    
    p=n;
   }
  remove_node(p);   
 }

/*****************************/
void list_of_list::remove()
 {
  list_of_list_node *t;

  t=current;

  if (t->kid) recursive_remove(t->kid);

  if (t->next) current=t->next; 
     else if (t->previous)
                current=t->previous;
          else {
                current=t->parent;
                t->parent->kid=NULL;
               }

  remove_node(t);

  delete t;
 }


/*****************************/
int  list_of_list::size()
 {
  if (current)
       if (current->object)
           {
            erreur=none;
            return current->size;
           }
       else {
             erreur=none;
             return 0;
            }
    
  else {
        erreur=list_is_empty;
        return -1;
       }
 }

/*****************************/
int  list_of_list::list()
 {
  if (current)
     {
      erreur=none;
      return (current->object==0);
     }
  else {
        erreur=list_is_empty;
        return 0;
       }
 }

/*****************************/
void *list_of_list::get_ref()
 {
  if (current)
       {
        if (current->contenu)
           {
            erreur=none;
            return current->contenu;
           }
        else {
              erreur=invalid_mode;
              return NULL;
             }
       }
  else {
        erreur=list_is_empty;
        return NULL;
       }
 }

/*****************************/
list_of_list_node *list_of_list::get_node_ref()
 {
  if (current)
      {
       erreur=none;
       return current;
      }
  else {
        erreur=list_is_empty;
        return NULL;
       }
 }

/*****************************/
int list_of_list::get(void *s)
 {
  if (current)
      {
       if (current->object)
          { 
           memcpy(s,current->contenu,current->size);
           erreur=none;
           return 1;
          }
       else { 
             erreur=invalid_mode;
             return 0;
            }
      }
  else {
        erreur=list_is_empty;
        return 0;
       }

 }

/*****************************/
int list_of_list::count()
 {
  return combien;
 }

/*****************************/
list_of_list::list_of_list()
 {
  state=0;
  combien=0;
  head=NULL;
  current=NULL;
  erreur=object_initialized_but_empty;
 }

/*****************************/
list_of_list::~list_of_list()
 {
  if (state) flush();
  state=0;
  combien=0;
  head=NULL;
  current=NULL;
  erreur=none;
 }


/* **************************************************************************************************************************** */

/*** Definitions de ticket ************************************************************/

ticket::ticket()
 {
  pointeur=NULL;
  state=0;
 }

ticket::~ticket()
 {
  pointeur=NULL;
  state=0;
 }


/* **************************************************************************************************************************** */

/*** definitions de string tree ******************************************************/

/*****************************/
string_tree::string_tree()
 {
  combien=0;
  uniqueness=0;
  oae=0;
  state=1;
  root=NULL;
  head=NULL;
  tail=NULL;
  erreur=object_initialized_but_empty;
 }

/*****************************/
string_tree::~string_tree()
 {
  if (state) flush();
  state=0;
 }


/*****************************/
void string_tree::flush()
 {
  reset();
  while (erreur==0) remove();
 }
                 
/*****************************/
void string_tree::reset()
 {
  current=head;
  erreur=none;
 }

/*****************************/
int string_tree::compare(const char *a, const char *b)
 {

  if (oae)
       return oae_compare(a,b);
  else return strcmp(a,b);
 }

/*****************************/
int  string_tree::seek(const char *s)
 {
  int i,ok;

  current=root;
  
  if (current)
     {
      ok=1;
      while ( (ok) && (i=compare(s,current->contenu)) )
       if (i<0)
           if (current->left) 
                current=current->left;
           else ok=0;
       else if (current->right)
                 current=current->right;
            else ok=0;

      return (i==0);
     }
  else 
     {
      erreur=list_is_empty;
      return 0;
     }
 }

/*****************************/
int  string_tree::seek_next()
 {
  if (current)
     {
      if (current->next)
         {
          current=current->next;
          return 1;
         }
      else 
         {
          erreur=end_of_list;
          return 0;
         }
     }
  else 
     {
      erreur=list_is_empty;
      return 0;
     }
 }

/*****************************/
int  string_tree::seek_previous()
 {
  if (current)
     {
      if (current->previous)
         {
          current=current->previous;
          return 1;
         }
      else 
         {
          erreur=head_of_list;
          return 0;
         }
     }
  else 
     {
      erreur=list_is_empty;
      return 0;
     }
 }

/*****************************/
int  string_tree::seek_first()
 {
  if (head)
     {
      current=head;
      return 1;
     }
  else 
     {
      erreur=list_is_empty;
      return 0;
     }
 }

/*****************************/
int  string_tree::seek_last()
 {
  if (tail)
     { 
      current=tail;
      return 1;
     }
  else 
     {
      erreur=list_is_empty;
      return 0;
     }  
 }

/*****************************/
void string_tree::put(const char *s)
 {
  add(s);
 }

/*****************************/
void string_tree::add(const char *s)
 {
  tree_node *p;


  if (root)
     if (!seek(s))
         {

          p=new(tree_node);
          p->parent=current;
          p->left=NULL;
          p->right=NULL;
          p->previous=NULL;
          p->next=NULL;
          p->count=1;
          p->size=strlen(s)+1;
          p->contenu=strdup(s);


          if (compare(s,current->contenu)<0)
              {
               current->left=p;

               if (head==current) head=p;

               p->previous=current->previous;
               if (p->previous) p->previous->next=p;
               current->previous=p;
               p->next=current;
               
              }
          else 
              {
               current->right=p;

               if (tail==current) tail=p;

               p->next=current->next;
               if (p->next) p->next->previous=p;
               current->next=p;
               p->previous=current;
              }
          combien++;
         }
     else current->count++;
  else 
     {
      p=new(tree_node);
      p->left=NULL;
      p->right=NULL;
      p->previous=NULL;
      p->parent=NULL;
      p->next=NULL;
      p->count=1;
      p->size=strlen(s)+1;
      p->contenu=strdup(s);

      root=p;
      head=p;
      tail=p;

      combien++;
     }
 }

/*****************************/
void string_tree::touch()
 {
  if (current)
     {
      current->count++;
      erreur=none;
     }
  else erreur=list_is_empty;
 }

/*****************************/
void string_tree::replace(const char *s)
 {
  remove();
  put(s);
 }

/*****************************/
void string_tree::movefrom(tree_node *p)
 {
  if (p->next) 
     current=p->next;
  else if (p->previous)
          current=p->previous;
       else current=NULL;
 }

/*****************************/
void string_tree::overthread(tree_node *p)
 {
  if (p->previous)
      { 
       p->previous->next=p->next;
       if (tail==p) tail=p->previous;
      }
  else head=p->next;

  if (p->next)
      {
       p->next->previous=p->previous;
       if (head==p) head=p->next;
      }
  else tail=p->previous;

  if (p->parent==NULL) root=NULL;
 }

/*****************************/
void string_tree::remove()
 {
  tree_node *p;
  int a;
  char *s;

  if (current)
     {

      p=current;

      if ( (current->left!=NULL) && (current->right==NULL) )
         {
          if (p->parent)
             if (p->parent->left==p) 
                  p->parent->left=p->left;
             else p->parent->right=p->left;
          p->left->parent=p->parent;
          overthread(p);
          movefrom(p);
          free(p->contenu);
          delete(p);
          combien--;
         }
      else if ( (current->left==NULL) && (current->right!=NULL) )
              {
               if (p->parent)
                  if (p->parent->left==p)
                       p->parent->left=p->right;
                  else p->parent->right=p->right;
               p->right->parent=p->parent;
               overthread(p);
               movefrom(p);
               free(p->contenu);
               delete(p);
               combien--;
              }
           else if ( (current->left==NULL) && (current->right==NULL) )
                   {
                    if (p->parent)
                       if (p->parent->left==p)
                            p->parent->left=NULL;
                       else p->parent->right=NULL;
                    overthread(p);
                    movefrom(p);
                    free(p->contenu);
                    delete(p);
                    combien--;
                   }
                else 
                   {

                    /* Trouver le plus petit noeud plus grand que current */

                    p=current->right;
                    while (p->left) p=p->left;

                    a=current->size;
                    current->size=p->size;
                    p->size=a;

                    s=current->contenu;
                    current->contenu=p->contenu;
                    p->contenu=s;

                    current=p;
                    remove();          
                   }

     }
  else erreur=list_is_empty;
 }


/*****************************/
char *string_tree::get_ref()
 {
  if (current)
     {
      erreur=0;
      return current->contenu;
     }
  else 
     {
      erreur=list_is_empty;
      return NULL;
     }
 }

/*****************************/
tree_node  *string_tree::get_node_ref()
 {
  if (current)
     return current;
  else 
     {
      erreur=list_is_empty;
      return NULL;
     }
 }

/*****************************/
int string_tree::get(char *s)
 {
  if (current)
     {
      strcpy(s,current->contenu);
      return 1;
     }
  else
     {
      erreur=list_is_empty;
      return 0;
     }
 }

/*****************************/
int string_tree::get_count()
 {
  if (current)
     return current->count;
  else 
     {
      erreur=list_is_empty;
      return 0;
     }
 }


/*****************************/
int string_tree::count()
 {
  return combien;
 }


/* **************************************************************************************************************************** */


/*** definitions de large_array *******************************************************/

/****************************************/
void large_array::init(int combien, int size)
    {
     int i;

     if (state) flush();

     _combien=combien;
     _size=size;
     table=(void **)malloc(combien*sizeof(void *));
     for (i=0;i<_combien;i++) table[i]=(void *)0;
     erreur=none;
     state=1;
    }

/****************************************/
void large_array::put(int n,void *p)
 {
  if (n<_combien)
     {
      if (table[n]==0) 
         {
          table[n]=malloc(_size);
          if (table[n]==(void *)0) 
             {
              erreur=rte_heap_overflow;
             }
        }
      memcpy(table[n],p,_size);
     }
  else erreur=rte_range_check_error;
 }

/****************************************/
void large_array::get(int n,void *p)
 {
  memcpy(p,table[n],_size);
 }

/****************************************/
void *large_array::ref(int n)
 {
  return table[n];
 }

/****************************************/
void large_array::flush()
 {
  int i;

  for (i=0; i<_combien; i++)
   if (table[i]) free(table[i]);

  free(table);

  state=0;
 }

/****************************************/
large_array::large_array()
 {
  state=0;
 }

/****************************************/
large_array::~large_array()
 {
  if (state) flush();
 }

/* **************************************************************************************************************************** */

/*** definitions de s_treille *********************************************************/

/****************************************/
s_treille::s_treille()
 {
  combien=0;
  c=(pTreille_tableau)0;
  sizeof_treille=0;
  erreur=object_initialized_but_empty;
 }

/****************************************/
s_treille::~s_treille()
 {
  free(c);
  erreur=object_not_initialized;
 }

/****************************************/
int  s_treille::count()
 {
  return combien;
 }

/****************************************/
void s_treille::init(int size,int maxx)
 {
  int t;
  noeud_de_s_treille p;

  sizeof_treille=size;
  c=(pTreille_tableau)malloc(sizeof_treille*sizeof(noeud_de_s_treille));
  if (c)
     {
      pointeur=-1;
      combien=min(maxx,256);

      p.frere=-1;
      p.parent=-1;
      p.fils=-1;
      p.count=0;

      for (t=0;t<maxx;t++)
       {
        p.occupant=(unsigned char)t;
        c[t]=p;
       }
      erreur=0;
     }
  else erreur=not_enough_memory;
 }

/****************************************/
void s_treille::flush()
 {
  free(c);
  s_treille();
 }

/****************************************/
void s_treille::reset()
 {
  pointeur=-1;
 }

/****************************************/
int s_treille::seek(unsigned char i)
 {
  if (pointeur==-1)
      {
       pointeur=i;
       return True;
      }
  else
      {
       while ( (c[pointeur].frere!=-1) &&
               (c[pointeur].occupant!=i))
               pointeur=c[pointeur].frere;

       return (c[pointeur].occupant==i);
      }
 }

/****************************************/
int s_treille::descend()
 {
  if (c[pointeur].fils!=-1)
     {
      pointeur=c[pointeur].fils;
      return True;
     }
  else return False;
 }


/****************************************/
int s_treille::add_frere(unsigned char i)
 {
  int a;
  if (pointeur!=-1)
      {
       a=c[pointeur].parent;
       while (c[pointeur].frere!=-1) pointeur=c[pointeur].frere;
       c[pointeur].frere=combien;
       pointeur=combien;
       c[pointeur].occupant=i;
       c[pointeur].parent=a;
       c[c[pointeur].parent].combien++;  /* Ok, meme pour les superparents */
       c[pointeur].frere=-1;
       c[pointeur].fils=-1;
       c[pointeur].count=0;
       c[pointeur].combien=0;  /* Il n'a pas encore de fils */
       combien++;
       return True;
      }
  else return False;
 }

/****************************************/
int s_treille::add_fils(unsigned char i)
 {
  int a;

  if  (c[pointeur].fils==-1)
      {
       a=pointeur;
       c[pointeur].fils=combien;
       c[pointeur].combien=1;     /* premier fils */
       pointeur=combien;
       c[pointeur].parent=a;
       c[pointeur].occupant=i;
       c[pointeur].frere=-1;
       c[pointeur].fils=-1;
       c[pointeur].count=0;
       c[pointeur].combien=0;    /* Il n'a pas encore de fils! */
       combien++;
       return True;
      }
  else
      {
       descend();
       return add_frere(i);
      }
 }

/****************************************/
int s_treille::libere(int nombre)
 {
  return conserve(sizeof_treille-nombre);
 }

/****************************************/
int s_treille::conserve(int nombre)
 {
  int i,t;

  i=nombre;

  for (t=0;t<i;t++)
   {
    if (c[t].frere>=i) c[t].frere=-1;
    if (c[t].fils>=i)  c[t].fils=-1;
    c[t].count=0;
   }
  combien=i;
  reset();
  return True;
 }

/****************************************/
int s_treille::full()
 {
  return (combien==sizeof_treille);
 }
/****************************************/
void s_treille::touch_node()
 {
  if (pointeur!=-1) c[pointeur].count++;
 }

/****************************************/
void s_treille::touch_word()
 {
  int i;

  i=pointeur;

  if (pointeur!=-1)
     do
      {
       c[i].count++;
       i=c[i].parent;
      }
     while (i!=-1);
 }

/****************************************/
void s_treille::extract(unsigned char *s, int position)
 {
  int i,j;
  j=position;
  i=0;
  do
   {
    s[i++]=c[j].occupant;
    j=c[j].parent;
   }
  while (j!=-1);
  s[i]=0;
 }

/* **************************************************************************************************************************** */

/*** definitions de string_list *******************************************************/

/****************************************/
string_list::string_list()
 {
  state=1;
  uniqueness=0;
  oae=0;
  combien=0;
  head=NULL;
  tail=NULL;
  current=NULL;
  erreur=object_initialized_but_empty;
 }

/****************************************/
string_list::~string_list()
 {
  if (state) flush();
  uniqueness=0;
  oae=0;
  combien=0;
  head=NULL;
  tail=NULL;
  current=NULL;
  erreur=object_not_initialized;
 }

/*****************************/
int string_list::compare(const char *a, const char *b)
 {
  if (oae==0)
       return strcmp(a,b);
  else return oae_compare(a,b);
 }

/****************************************/
int string_list::count()
 {
  return combien;
 }

/****************************************/
void string_list::flush()
 {
  reset();
  while (remove()==0);
  state=0;
  head=NULL;
  tail=NULL;
 }

/****************************************/
void string_list::reset()
 {
  current=head;
  erreur=0;
 }

/****************************************/
int string_list::seek_next()
 {
  if (current->next)
     {
      current=current->next;
      erreur=0;
     }
  else erreur=end_of_list;
  return erreur;
 }

/****************************************/
int string_list::seek_previous()
 {
  if (current->previous)
     {
      current=current->previous;
      erreur=0;
     }
  else erreur=head_of_list;
  return erreur;
 }

/****************************************/
void string_list::seek_last()
 {
  current=tail;
 }

/****************************************/
void string_list::append(const char *s)
 {
  add(s);
 }

/****************************************/
void string_list::add(const char *s)
 {
  list_node *p;

  p=(list_node *)malloc(sizeof(list_node));
  p->size=strlen(s)+1;  
  p->count=0;
  p->contenu=(char *)malloc(p->size);
  strcpy(p->contenu,s);
  p->next=NULL;
  p->previous=NULL;
  
  if (tail)
     {
      tail->next=p;
      p->previous=tail;
      tail=p;
     }
  else 
     {
      p->previous=NULL;
      head=p;
      tail=p;
     }
  combien++;
 }

/****************************************/
void string_list::touch()
 {
  if (current)
     { 
      current->count++;
     }
  else erreur=list_is_empty;
 }

/****************************************/
void string_list::replace(const char *s)
 {
  if (current)
       {
        free(current->contenu);
        current->contenu=(char *)malloc(strlen(s)+1);
        strcpy(current->contenu,s);
        current->count=0;
       }
  else erreur=list_is_empty;
 }

/****************************************/
void string_list::insere_avant(const char *s)
 {
  list_node *p;

  p=(list_node *)malloc(sizeof(list_node));
  p->next=NULL;
  p->previous=NULL;
  p->count=1;
  p->size=strlen(s)+1;
  p->contenu=(char *)malloc(p->size);
  strcpy(p->contenu,s);

  if (current==head) head=p;
  if (current->previous) current->previous->next= p;

  p->previous=current->previous;
  p->next=current;
  current->previous=p;
  erreur=0;
  combien++;
 }

/****************************************/
void string_list::insere_apres(const char *s)
 {
  list_node *p;

  p=(list_node *)malloc(sizeof(list_node));
  p->count=1;
  p->next=NULL;
  p->previous=NULL;
  p->size=strlen(s)+1;
  p->contenu=(char *)malloc(p->size);
  strcpy(p->contenu,s);

  if (current==tail) tail=p;
  if (current->next) current->next->previous= p;

  p->next=current->next;
  current->next=p;
  p->previous=current;

  erreur=0;
  combien++;
 }

/****************************************/
int string_list::seek(const char *s)
 {
  int i;

  reset();
  while ( (erreur==0) && ((i=compare(current->contenu,s))<0) )
   {
    seek_next();
   }

  return (i==0);
 }

/****************************************/
void string_list::put(const char *s)
 {
  int i;

  if (combien==0) 
     add(s);
  else
     {
      reset();
      while ( (erreur==0) && ((i=compare(current->contenu,s))<0))
       {
        seek_next();
       }

      if (i) 
           insere_avant(s);
      else if (!uniqueness) 
              insere_avant(s);
           else 
              {
               current->count++;
               erreur=duplicate_identifier_in_list;
              }
     }
 }

/****************************************/
void string_list::reverse_put(const char *s)
 {
  int i;

  if (combien==0) 
     add(s);
  else
     {
      seek_last();
      while ( (erreur==0) && ((i=compare(current->contenu,s))>0))
       {
        seek_previous();
       }

      if (i) 
           insere_apres(s);
      else if (!uniqueness) 
                   insere_apres(s);
              else erreur=duplicate_identifier_in_list;
     }
 }

/****************************************/
int string_list::remove()
 {
  list_node *t;

  if (combien)
     {
      if (current==head)
          {
           t=current->next;
           head=t;
          }
      else if (current==tail)
           {
            t=current->previous;
            tail=t; 
           }
           else t=current->previous;

      if (current->previous) current->previous->next=current->next;
      if (current->next) current->next->previous=current->previous;

      free(current->contenu);
      free(current);

      current=t;
      erreur=0;
      combien--;
     }
  else erreur = list_is_empty;

  return erreur;
 }

/****************************************/
char *string_list::get_ref()
 {
  return current->contenu;
 }

/****************************************/
list_node *string_list::get_node_ref()
 {
  return current;
 }

/****************************************/
int string_list::get_count()
 {
  if (current)
      {
       erreur=0;
       return current->count;
      }
  else
      {
       erreur=list_is_empty;
       return 0;
      }
 }

/****************************************/
int string_list::get(char *s)
 {
  if (current)
      {
       compare(s,current->contenu);
       erreur=0;
      }
  else erreur= list_is_empty;
  return erreur;
 }

/****************************************/
void  string_list::quick_sort_ascendant(list_node *tete,list_node *queue,int lo,int hi)
 {
  char       p[255];
  list_node *px;
  list_node *py;
  int        x,y,i;
       
  x=lo;
  y=hi;
  px=tete;
  for (i=x; i < ((x+y)/2); i++) px=px->next;
  strcpy(p,px->contenu);

  px=tete;
  py=queue;
  do
   {
    while (compare(px->contenu,p)<0) 
     {
      x++;
      px=px->next;
     }

    while (compare(p,py->contenu)<0) 
     {
      y--;
      py=py->previous;
     }

    if (x<=y)
        {
         exchange(px,py);
         x++;
         y--;
         px=px->next;
         py=py->previous;
        }
   }
  while (x<=y);
  if (lo<y) quick_sort_ascendant(tete,py,lo,y);
  if (hi>x) quick_sort_ascendant(px,queue,x,hi);
 }

/****************************************/
void string_list::exchange(list_node *a, list_node *b) 
 {
  char *tx;
  int   i;

  i=a->size;
  a->size=b->size;
  b->size=i;

  tx=a->contenu;
  a->contenu=b->contenu;
  b->contenu=tx;

 }

/****************************************/
void  string_list::quick_sort_descendant(list_node *tete,list_node *queue,int lo,int hi)
 {
  char      p[255];
  list_node *px;
  list_node *py;
  int       x,y,i;
        
  x=lo;
  y=hi;
  px=tete;
  for (i=x; i<((x+y)/2); i++) px=px->next;

  strcpy(p,px->contenu);

  px=tete;
  py=queue;

  do
   {
    while (compare(p,px->contenu)<0) 
     {
      x++;
      px=px->next;
     }

    while (compare(py->contenu,p)<0) 
     {
      y--;
      py=py->previous;
     }

    if (x<=y)
        {
         exchange(px,py);
         x++;
         y--;
         px=px->next;
         py=py->previous;
        }
   }
  while (x<=y);
  if (lo<y) quick_sort_descendant(tete,py,lo,y);
  if (hi>x) quick_sort_descendant(px,queue,x,hi);
 }


/****************************************/
int string_list::sort(int sens)
 {
  if (combien)
     {
      if (sens==0)
           quick_sort_ascendant(head,tail,1,combien);
      else quick_sort_descendant(head,tail,1,combien);
      erreur=0;
     }
  else erreur = list_is_empty;
  return erreur;
 }






// --------------- #include "fwf.cpp"




/* ******************************************************************************************


  FWF: files without format




  by Steven Pigeon



  This class provides for any number of object to be saved in a file and then retreived.
  


  ****************************************************************************************** */



  const char fwf_signature[4] = {'F','W','F','\n'};

  const unsigned char fwf_version = (unsigned char)0x10;  /* High nibble = major version, low nibble = minor version */

  /* 
  
     The registry class provides indentifactions
     for objects of any kind (classes or other types)

     note: The names of classes are expected to match EXACTLY.

     The first entry will have then number 0, the second 1, etc.

     */


  class registry
         {
          private: string_list registry_list;


                   int suck(FILE *f,char *buff);                  /* reads a \n terminated string from a binary file. */

          public:  int erreur;


                   /* registry functions */

                   int count();                                   /* how many classes are registered?                      */
                   int register_class(const char *class_name);    /* a class name, get a registry number , watch 'erreur'  */
                   char *class_name(const int registry_number);   /* for a registry number, get a class_name, return the null string if invalid */
                   int get_registry(const char *class_name);


                   int load(const char *filename);                /* loads registry information from a file named filename */
                   int load(FILE *f);                             /* loads registry information from an opened file f      */
                   int load_binary(FILE *f);                      /* loads from a binary file */
                   int load_binary(const char *filename);         /* loads from a binary file */

                   int size();                                    /* Returns how many bytes it would use on disk        */

                   int save(const char *filename);                /* saves registry information into a file named filename */
                   int save(FILE *f);                             /* saves registry information into an opened file f      */ 


                   void flush();                                  /* destroys registry info (in memory)                    */


                   registry();
                   ~registry();
         };



 /* *********************************************************************************************************************

     The FWF class let you load/save a collection with indexing.

    ********************************************************************************************************************* */


 /* ********************************************************************************************************************* */

 typedef struct
          {
            char signature[4];                /* FWF\n */
            unsigned char version;
            char endian;                      /* 0 = intel (small), 1= motorola (big), 2= else! */
            int  number_of_objects;           /* well... */
            int  pointer_to_index;            /* index is at the tail of file */
            int  pointer_to_registry;         /* registry is after index, at the tail of file */
            int  pointer_to_comments;         /* You can optionnaly add comments to a file. (string list) */
            int  length_of_comments;
          } fwf_header;

 /* ********************************************************************************************************************* */

 typedef struct
          { 
            int registry_number;              /* object identification                   */
            int size;                         /* its size (so that you could block-read) */
            int next;                         /* where's next? it's should be here+size! */
          } fwf_list_element;


 /* ********************************************************************************************************************* */

 typedef struct
          { 
            int registry_number;              /* What type? */
            int position;                     /* where's that? */
          } fwf_index_element;

 /* ********************************************************************************************************************* */


// explicit instanciation of dynamic_array

#ifdef __GNUG__
template class dynamic_array<fwf_index_element>;
#endif


 class FWF
        {
         private: int                               state;
                  registry                          registry_list;  /* the registry list: the names and numbers of class */
                  dynamic_array<fwf_index_element>  index;          /* where's what */
                  int                               combien;        /* combien d'objets     */
                  int                               next;           /* Where should be next */
                  int                               write_mode;     /* 0 = opened in read mode, 1=opened in write mode */
                  int                               comments;
                  int                               comments_ptr;

                  int   save_index();                               /* 1 if successful, 0 otherwise (called by close()) */
                                    
         public:  int   erreur;
                  FILE  *fwf;

                  int   registry_count();
                  int   registry_size();
                  int   register_class(const char *classname);
                  int   get_registry(const char *classname);         /* returns -1 if classname is unknown. */
                  char *class_name(const int registry_number);

                  int   get_save_pointer(int &number, int registry_number,int size_of_object_on_disk); /* positions fwf to the position for writing an object of type registry number and size size_of_..., returns number of object */
                  int   get_load_pointer(int registry_number,int number);  /* returns the pointer to the number^th object of type registry_number, -1 if failed */
                  int   get_load_pointer(int number);                      /* returns the pointer to the number^th object, -1 if failed */
                  fwf_index_element  get_object_info(int number);          /* returns <registry_number,position> */

                  int   get_comments(char *s);                             /* 1 if there's comments, and dumps it into s, 0 if there's no comments, s untouched. */

                  int   open(const char *filename, const char *mode);      /* 1 if successful, 0 otherwise */
                  int   mode();                                            /* 0 if in read mode, 1 if in write mode */
                  void  close(const char *comments);                       /* see erreur for results. With no comments, set comments=NULL */

                  void  flush();                                           /* destructeur */

                  FWF();
                  ~FWF();
        };
  

/***********************************************************************************************************************************/

/*** FWF *******************************************************************************************/

/**************************************/
int   FWF::registry_count()
 {
  return registry_list.count();  
 }

/**************************************/
int   FWF::registry_size()
 { 
  return registry_list.size();
 }

/**************************************/
int   FWF::get_registry(const char *classname)
 {
  return registry_list.get_registry(classname);
 }

/**************************************/
int   FWF::register_class(const char *classname)
 {
  return registry_list.register_class(classname);
 }

/**************************************/
char *FWF::class_name(const int registry_number)
 {
  if (registry_number<registry_list.count())
     {
      erreur=none;
      return registry_list.class_name(registry_number);
     }
  else {
        erreur=range_check_error;
        return (char *)NULL;
       }
 }


/**************************************/
int   FWF::save_index()
 { 
  fseek(fwf,next,0);

  if ( (fwrite(&index[0],sizeof(fwf_index_element),combien,fwf)==(size_t)combien) &&
       (registry_list.save(fwf)) 
       )
     {
      erreur=none;
      return 1;
     }
  else {
        erreur=object_save_failed;
        return 0;
       }
 }

/**************************************/
int   FWF::get_comments(char *s)
 { 
  if (state)
     {
      if (comments)
          { 
           fseek(fwf,comments_ptr,0);
           fread(s,comments,1,fwf);
           return 1;
          }
      else {
            erreur=none;
            return 0;
           }
     }
  else {
        erreur=object_initialized_but_empty;
        return 0;
       }
 }

/**************************************/
int   FWF::get_save_pointer(int &number, int registry_number,int size_of_object_on_disk)
 { 
  int i;
  fwf_list_element e;


  fseek(fwf,next,0); /* Next always points to the location, then writes the placeholder */

  index[combien].registry_number=registry_number;
  index[combien].position=next;
  
  i=next+sizeof(fwf_list_element);
  next+=sizeof(fwf_list_element)+size_of_object_on_disk;
  
  e.registry_number=registry_number;
  e.size=size_of_object_on_disk;
  e.next=next;

  fwrite(&e,sizeof(fwf_list_element),1,fwf);

  number=combien++;
  return i;
 }

/**************************************/
fwf_index_element FWF::get_object_info(int number)
 {
  if (number<combien)
      {
       erreur=none;
       return index[number];
      }
  else {
        fwf_index_element a;
        a.position=-1;
        a.registry_number=-1;
        erreur=range_check_error;
        return a;
       }
 }

/**************************************/
int FWF::mode()
 {
  return write_mode;
 }

/**************************************/
int   FWF::get_load_pointer(int number)
 { 
  if (number<combien)
     {
      erreur=none;
      return index[number].position+sizeof(fwf_list_element);
     }
  else {
        erreur=range_check_error;
        return -1;
       }
 }


/**************************************/
int   FWF::get_load_pointer(int registry_number,int number)
 { 
  int i,j,found;

  if (number<combien)
     {
      found=0;
      i=0;
      j=-1;
      do
        {
         j++;
         if (index[j].registry_number==registry_number) 
             {
              found=(i==number);
              i++;
             }
        }
      while ( (!found) && ((j+1)<combien) );

      if (found) 
          {
           erreur=none;
           return index[j].position + sizeof(fwf_list_element);
          }
      else {
            erreur=range_check_error;
            return -1;
           }
     }
  else {
        erreur=range_check_error;
        return -1;
       }
    
 }

/**************************************/
int   FWF::open(const char *filename, const char *mode)
 {
  fwf_header a;
  int i;


  fwf=fopen(filename,mode);

  if (fwf)
     {
      if (mode[0]=='w')
          {
           for (i=0;i<4;i++) a.signature[i]=fwf_signature[i];
           a.version = fwf_version;
           a.endian = indien_int();
           a.number_of_objects=0;
           a.pointer_to_index=0;
           a.pointer_to_registry=0;
           a.pointer_to_comments=0;
           a.length_of_comments=0;
           fwrite(&a,sizeof(a),1,fwf);
           write_mode = 1;
          }
      else {
            fread(&a,sizeof(a),1,fwf);
            combien=a.number_of_objects;
            comments=a.length_of_comments;
            comments_ptr=a.pointer_to_comments;
            fseek(fwf,a.pointer_to_index,0);
            index.set_size(combien);
            fread(&index[0],sizeof(fwf_index_element),combien,fwf);
            registry_list.load_binary(fwf);
            write_mode=0;
           }

      next=sizeof(fwf_header);  /* next place to read or write */
      fseek(fwf,next,0);

      state=1;

      return 1;
     }
  else {
        erreur=rte_file_not_open;
        return 0;
       }
 }

/**************************************/
void FWF::close(const char *comments)
 {
  if (state)
      {
       fwf_header a;
       int i;

       if (write_mode) 
           {
            fseek(fwf,0,0);

            for (i=0;i<4;i++) a.signature[i]=fwf_signature[i];
            a.endian=indien_int();
            a.number_of_objects=combien;
            a.pointer_to_index=next;
            a.pointer_to_registry=next+combien*sizeof(fwf_index_element);
            if (comments)
                 {
                  a.pointer_to_comments=next+combien*sizeof(fwf_index_element)+registry_list.size();
                  a.length_of_comments=strlen(comments)+1;
                 }
            else { 
                  a.pointer_to_comments=NULL;
                  a.length_of_comments=0;
                 }

            fwrite(&a,sizeof(a),1,fwf);

            fseek(fwf,next,0);
            save_index();
            if (comments) fwrite(comments,strlen(comments)+1,1,fwf);
            
            
           }

       fclose(fwf);
       index.flush();
       registry_list.flush();
       state=0;
       combien=0;
       next=0;
      }
  erreur=none;
 }

/**************************************/
void  FWF::flush()
 {
  if (state) close(NULL);
 }

/**************************************/
FWF::FWF()
 {
  state=0;
  combien=0;
  next=0;
  erreur=object_initialized_but_empty;
 }

/**************************************/
FWF::~FWF()
 {
  if (state) flush();
  state=0;
  erreur=object_not_initialized;
 }


/*** registry **************************************************************************************/


/**************************************/
int registry::count()
 { 
  return registry_list.count();
 }

/**************************************/
int registry::register_class(const char *class_name)
 {
  int i;
  int j=0;

  registry_list.reset();

  if (registry_list.count())
     {
      while ( (registry_list.erreur==0) && 
              (i=strcmp(registry_list.get_ref(),class_name)) )
       {
        registry_list.seek_next();
        j++;
       }
      }
  else i=1;

  if (i==0) 
       erreur = registry_existing_entry;
  else {
        registry_list.append(class_name); 
        erreur = registry_new_entry;
       }

  return j;

 }

/**************************************/
int registry::get_registry(const char *class_name) 
 { 
  int i;
  int j=0;

  registry_list.reset();

  if (registry_list.count())
    {
      while ((registry_list.erreur==0) &&
             (i=strcmp(registry_list.get_ref(),class_name)) )
        {
          j++;
          registry_list.seek_next();
        }
      if (i==0) 
        {
          erreur=none;
          return j; 
        }
    }
  erreur=registry_unknown_class;
  return -1;
 }

/**************************************/
char *registry::class_name(const int registry_number)
 {
  int i;
  
  if (registry_number<registry_list.count())
       {
        registry_list.reset();
        for (i=0;i<registry_number;i++) registry_list.seek_next();

        erreur = none;
        return registry_list.get_ref();
       }
  else { 
        erreur= range_check_error;
        return (char *)NULL;
       }
 }

/**************************************/
int registry::load(const char *filename)
 {
  FILE *f;
  int  i;

  if ((f=fopen(filename,"r")))
     {
      i=load(f);
      fclose(f);
      return i;
     }
  else {
        erreur=rte_file_not_found;
        return 0;
       }
 }

/**************************************/
int registry::load(FILE *f)
 { 
  int i=0;
  char buffer[1024];

  registry_list.flush();

  while (fscanf(f,"[^\n]\n",buffer))
   {
    i++;
    registry_list.append(buffer);
   }
  if (i==0) erreur= file_may_be_crippled;
  return (i!=0);
 }

/**************************************/
int registry::load_binary(const char *filename)
 {
  FILE *f;
  int  i;

  if ((f=fopen(filename,"rb")))
     {
      i=load_binary(f);
      fclose(f);
      return i;
     }
  else {
        erreur=rte_file_not_found;
        return 0;
       }
}

/**************************************/
int registry::suck(FILE *f, char *buff)
 {
  int i=0;
  char c;

  do
   {
    fread(&c,1,1,f);
    buff[i++]=c;
   }
  while ((c!='\n') && (!feof(f)) );
  i--;
  buff[i]=0;

  return (i>0);
 }

/**************************************/
int registry::load_binary(FILE *f)
 { 
  int i=0;
  char buffer[1024];

  registry_list.flush();

  while (suck(f,buffer))
   {
    i++;
    registry_list.append(buffer);
   }
  if (i==0) 
      erreur= file_may_be_crippled;
  else erreur=none;
  return (i!=0);
 }


/**************************************/
int registry::size() 
 {
  int i=0;


  if (registry_list.count())
     {
      registry_list.reset();
      do
       {
        i+=strlen(registry_list.get_ref())+1;
        registry_list.seek_next();
       }
      while (registry_list.erreur==none);
     }
  
  return i+1;  
 }


/**************************************/
int registry::save(const char *filename)
 {
  FILE *f;
  int i;

  if ((f=fopen(filename,"w"))) 
     {
      i=save(f);
      fclose(f);
      return i;
     }
  else {
        erreur=rte_file_not_open;
        return 0;
       }
 }


/**************************************/
int registry::save(FILE *f)
 {
  int i,j,k=0;

  i=registry_list.count();

  registry_list.reset();
  for (j=0;j<i;j++) 
       {
        k+=(0!=fprintf(f,"%s\n",registry_list.get_ref()));
        registry_list.seek_next();
       }
  k+=(0!=fprintf(f,"\n"));

  return (k==(i+1));
 }


/**************************************/
registry::registry()
 {
 }


/**************************************/
void registry::flush()
 {
  registry_list.flush();
 }

/**************************************/
registry::~registry()
 {
 }





// C interface for calling FWF from SN 

extern "C" {

// Create a new FWF (file without format), with the given file name.
// The mode should either be "wb" (to write) or "rb" (to read).
FWF* new_fwf(char *fname, char* mode) {
    FWF* f = new FWF;
    f->open(fname,mode);
    return f;
}

// Return FILE* associated to FWF. Used to write stuff in the FWF.
FILE* fwf_fp(FWF* f) { return f->fwf; }

// Register the given class name in registry of class names kept in the FWF.
// Return a handle for that name.
int fwf_register_class(FWF* f, char* class_name) {
    return f->register_class(class_name);
}

// Return handle of a class that has already been registered.
int fwf_get_registry(FWF* f, char* class_name) {
    return f->get_registry(class_name);
}

// Return class name given its handle
char *fwf_class_name(FWF* f, int class_handle) {
    return f->class_name(class_handle);
}

// Get position in file where to put stuff of given class and size.
int fwf_get_save_pointer(FWF* f, int* handle, int class_handle, int size) {
    return f->get_save_pointer(*handle, class_handle, size);
}

// Get position in file where to get stuff with given handle. Return -1 if failed.
int fwf_get_load_pointer(FWF* f, int handle) {
    return f->get_load_pointer(handle);
}

// Get position in file where to get i-th object of given class. Return -1 if failed.
int fwf_get_load_pointer_wrt_class(FWF* f, int class_handle, int i) {
    return f->get_load_pointer(class_handle, i);
}

// Get class handle and position of an object from its handle.
// Return class handle and set position argument.
int fwf_get_info(FWF* f, int handle, int *position) {
    fwf_index_element o = f->get_object_info(handle);
    *position = o.position;
    return o.registry_number;
}


// Close file and write some comments. The FWF is de-allocated.
void fwf_close(FWF *f, char* comments) {
    f->close(comments);
    delete f;
}

// Get comments saved with fwf_close. The argument must point to a large enough area...
void fwf_get_comments(FWF *f, char* comments) {
    f->get_comments(comments);
}


}

