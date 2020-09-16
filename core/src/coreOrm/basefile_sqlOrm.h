// generated by sqlpp11-ddl2cpp 008-basefile.sql basefile_sqlOrm doodle
#ifndef DOODLE_BASEFILE_SQLORM_H
#define DOODLE_BASEFILE_SQLORM_H

#include <sqlpp11/table.h>
#include <sqlpp11/data_types.h>
#include <sqlpp11/char_sequence.h>

namespace doodle
{
  namespace Basefile_
  {
    struct Id
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "id";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T id;
            T& operator()() { return id; }
            const T& operator()() const { return id; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::must_not_insert, sqlpp::tag::must_not_update, sqlpp::tag::can_be_null>;
    };
    struct File
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "file";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T file;
            T& operator()() { return file; }
            const T& operator()() const { return file; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::can_be_null>;
    };
    struct FileSuffixes
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "fileSuffixes";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T fileSuffixes;
            T& operator()() { return fileSuffixes; }
            const T& operator()() const { return fileSuffixes; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::can_be_null>;
    };
    struct User
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "user";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T user;
            T& operator()() { return user; }
            const T& operator()() const { return user; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::can_be_null>;
    };
    struct Version
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "version";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T version;
            T& operator()() { return version; }
            const T& operator()() const { return version; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::can_be_null>;
    };
    struct _filePath_
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "_file_path_";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T FilePath_;
            T& operator()() { return FilePath_; }
            const T& operator()() const { return FilePath_; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::can_be_null>;
    };
    struct Infor
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "infor";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T infor;
            T& operator()() { return infor; }
            const T& operator()() const { return infor; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::can_be_null>;
    };
    struct Filestate
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "filestate";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T filestate;
            T& operator()() { return filestate; }
            const T& operator()() const { return filestate; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::varchar, sqlpp::tag::can_be_null>;
    };
    struct Filetime
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "filetime";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T filetime;
            T& operator()() { return filetime; }
            const T& operator()() const { return filetime; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
    };
    struct _Episodes_
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "__episodes__";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T _episodes_;
            T& operator()() { return _episodes_; }
            const T& operator()() const { return _episodes_; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::can_be_null>;
    };
    struct _Shot_
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "__shot__";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T _shot_;
            T& operator()() { return _shot_; }
            const T& operator()() const { return _shot_; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::can_be_null>;
    };
    struct _FileClass_
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "__file_class__";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T _fileClass_;
            T& operator()() { return _fileClass_; }
            const T& operator()() const { return _fileClass_; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::can_be_null>;
    };
    struct _FileType_
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "__file_type__";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T _fileType_;
            T& operator()() { return _fileType_; }
            const T& operator()() const { return _fileType_; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::can_be_null>;
    };
    struct _AssClass_
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "__ass_class__";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T _assClass_;
            T& operator()() { return _assClass_; }
            const T& operator()() const { return _assClass_; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::can_be_null>;
    };
  } // namespace Basefile_

  struct Basefile: sqlpp::table_t<Basefile,
               Basefile_::Id,
               Basefile_::File,
               Basefile_::FileSuffixes,
               Basefile_::User,
               Basefile_::Version,
               Basefile_::_filePath_,
               Basefile_::Infor,
               Basefile_::Filestate,
               Basefile_::Filetime,
               Basefile_::_Episodes_,
               Basefile_::_Shot_,
               Basefile_::_FileClass_,
               Basefile_::_FileType_,
               Basefile_::_AssClass_>
  {
    struct _alias_t
    {
      static constexpr const char _literal[] =  "basefile";
      using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
      template<typename T>
      struct _member_t
      {
        T basefile;
        T& operator()() { return basefile; }
        const T& operator()() const { return basefile; }
      };
    };
  };
} // namespace doodle
#endif
