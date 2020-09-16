// generated by sqlpp11-ddl2cpp 001-episodes.sql episodes_sqlOrm doodle
#ifndef DOODLE_EPISODES_SQLORM_H
#define DOODLE_EPISODES_SQLORM_H

#include <sqlpp11/table.h>
#include <sqlpp11/data_types.h>
#include <sqlpp11/char_sequence.h>

namespace doodle
{
  namespace Episodes_
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
    struct Episodes
    {
      struct _alias_t
      {
        static constexpr const char _literal[] =  "episodes";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template<typename T>
        struct _member_t
          {
            T episodes;
            T& operator()() { return episodes; }
            const T& operator()() const { return episodes; }
          };
      };
      using _traits = sqlpp::make_traits<sqlpp::smallint, sqlpp::tag::can_be_null>;
    };
  } // namespace Episodes_

  struct Episodes: sqlpp::table_t<Episodes,
               Episodes_::Id,
               Episodes_::Episodes>
  {
    struct _alias_t
    {
      static constexpr const char _literal[] =  "episodes";
      using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
      template<typename T>
      struct _member_t
      {
        T episodes;
        T& operator()() { return episodes; }
        const T& operator()() const { return episodes; }
      };
    };
  };
} // namespace doodle
#endif
