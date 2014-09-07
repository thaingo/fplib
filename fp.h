//
//  fp.h
//  fplib
//
//  Created by Bao Thai Ngo on 7/08/2014.
//
//

#ifndef fplib_fp_h
#define fplib_fp_h

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <omp.h>
#include <sstream>
#include <functional>

#ifdef   FP_DEFINE
#include <myserializer/myserializer.hpp>
namespace fp {
    using myserializer::print;
}
#endif

#if defined(_NDEBUG) || defined(NDEBUG)
#define FP_DEBUG(...)
#define FP_RELEASE(...) __VA_ARGS__
#else
#define FP_DEBUG(...)   __VA_ARGS__
#define FP_RELEASE(...)
#endif

namespace fp {
    
    template<typename V>             using set = std::set<V>;   // unordered_set
    template<typename K, typename V> using map = std::map<K,V>; // unordered_map
    using type = unsigned;
    
    // fp::helpers
    
    template<typename T>
    T zero() {
        return std::pair<T,T>().first;
    }
    template<typename T>
    T one() {
        return !zero<T>();
    }
    
    template<typename T>
    T zero( const T &t ) {
        return zero<T>();
    }
    template<typename T>
    T one( const T &t ) {
        return one<T>();
    }
    
    template<typename T>
    bool is_true( const T &t ) {
        return t != zero<T>();
    }
    template<typename T>
    bool is_false( const T &t ) {
        return !is_true<T>(t);
    }
    
    template<typename T>
    void reset( T &t ) {
        t = zero<T>();
    }
    
    template<typename T>
    T &invalid() {
        static T t;
        return reset(t), t;
    }
    template<typename T>
    T &invalid( const T &t ) {
        return invalid<T>();
    }
    
    // fp::id
    
    template<typename T = type>
    T none() {
        return invalid<T>();
    }
    
    template<typename T = type>
    T &id() {
        static T _id = none();
        return ++_id;
    }
    
    // fp::entity
    
    std::string dump( const type & );
    
    class entity {
    public:
        type id;
        entity( const type &id_ = fp::id() ) : id(id_)
        {}
        operator type const &() const {
            return id;
        }
        operator type &() {
            return id;
        }
        template<typename component>
        typename decltype(component::value_type) &operator []( const component &t ) const {
            return fp::add<component>(id), fp::get<component>(id);
        }
        std::string str() const {
            return fp::dump(id);
        }
    };
    
    // fp::component
    
    enum GROUPBY_MODE {
        JOIN = 0, MERGE = 1, EXCLUDE = 2
    };
    
    template<typename T>
    inline fp::set<entity> &any() {
        static fp::set<entity> entities;
        return entities;
    }
    template<int MODE>
    inline fp::set<entity> group_by( const fp::set<entity> &A, const fp::set<entity> &B ) {
        const fp::set<entity> *tiny, *large;
        if( A.size() < B.size() ) tiny = &A, large = &B;
        else                      tiny = &B, large = &A;
        fp::set<entity> newset;  // union first, then difference, then intersection
        /**/ if (MODE == MERGE)   { newset = *large; for( auto &id : *tiny ) newset.insert(id); }
        else if (MODE == EXCLUDE) { newset = *large; for( auto &id : *tiny ) newset.erase (id); }
        else { for( auto &id : *tiny ) if( large->find(id) != large->end() ) newset.insert(id); }
        return newset;
    }
    
    // sugars {
    template<class T>                            fp::set< entity > join()   { return any<T>();                                  }
    template<class T, class U>                   fp::set< entity > join()   { return group_by<JOIN>( any<T>(), any<U>() );      }
    template<class T, class U, class V>          fp::set< entity > join()   { return group_by<JOIN>( any<T>(), join<U,V>() );   }
    template<class T, class U, class V, class W> fp::set< entity > join()   { return group_by<JOIN>( any<T>(), join<U,V,W>() ); }
    template<class T> fp::set<entity> exclude( const fp::set<entity> &B ) { return group_by<EXCLUDE>( any<T>(), B); }
    template<class T, class U> fp::set<entity> join( const T &t, const U &u ) { return join<T,U>(); }
    // }
    
    using system = std::function<bool(double,float)>;
    
    // parallelize
#   ifdef _MSC_VER
#       define OMP_PARA_INTERNAL __pragma(omp parallel for)
#   else
#       define OMP_PARA_INTERNAL _Pragma("omp parallel for") // C99
#   endif
#   define parallelize(id, sys) while(1) { \
const std::vector<type> vsys( sys.begin(), sys.end() ); \
const int end = vsys.size(); int it; \
OMP_PARA_INTERNAL for( it = 0; it < end; ++it ) { \
auto &id = vsys[it];
    // [...]
#   define pend \
} break; }
    
    template<typename T>
    fp::map< type, T > &components() {
        static fp::map< type, T > objects;
        return objects;
    }
    template<typename T>
    bool has( type id ) {
        return components<T>().find( id ) != components<T>().end();
    }
    template<typename T>
    inline decltype(T::value_type) &get( type id ) {
        FP_DEBUG(
                   // safe
                   static decltype(T::value_type) invalid, reset;
                   return has<T>(id) ? components<T>()[id].value_type : invalid = reset;
                   )
        FP_RELEASE(
                     // fast
                     return components<T>()[id].value_type;
                     )
    }
    template<typename T>
    inline decltype(T::value_type) &add( type id ) {
        any<T>().insert( id );
        components<T>()[id] = components<T>()[id];
        return get<T>(id);
    }
    template<typename T>
    inline bool del( type id ) {
        add<T>(id);
        components<T>().erase( id );
        any<T>().erase( id );
        return !has<T>( id );
    }
    struct icomponent {
        virtual ~icomponent() {}
        virtual void purge( type ) const = 0;
        virtual void swap( type, type ) const = 0;
        virtual void merge( type, type ) const = 0;
        virtual void copy( type, type ) const = 0;
        virtual void dump( std::ostream &, type ) const = 0;
    };
    static inline
    std::vector<const icomponent*> &registered() {
        static std::vector<const icomponent*> vector;
        return vector;
    }
    template<type NAME, typename T>
    struct component : icomponent {
        T value_type;
        component() {
            static struct registerme {
                registerme() {
                    registered().push_back( new component() );
                }
            } _;
        }
        
        // sugars {
        const component &operator+=( type id ) const {
            return add<component>(id), *this;
        }
        T &operator[]( type id ) const {
            return operator+=(id), get<component>(id);
        }
        // }
        
        virtual void purge( type id ) const {
            del<component>(id);
        }
        virtual void swap( type to, type from ) const {
            FP_DEBUG(
                       // safe
                       if( has<component>(to) && has<component>(from) ) {
                           std::swap( get<component>(to), get<component>(from) );
                       }
                       )
            FP_RELEASE(
                         // fast
                         std::swap( get<component>(to), get<component>(from) );
                         )
        }
        virtual void merge( type to, type from ) const {
            add<component>(to) = get<component>(from);
        }
        virtual void copy( type to, type from ) const {
            if( has<component>(from) ) {
                merge( to, from );
            } else {
                purge( to );
            }
        }
        virtual void dump( std::ostream &os, type id ) const {
#ifdef FP_DEFINE
            std::stringstream is;
            os << ( fp::print( get<component>(id), is ), is.str() ) << ',';
#else
            std::stringstream ss;
            ss << get<component>(id) << ',';
            os << ss.str();
#endif
        }
        inline typename T &operator()( type id ) {
            return get<component>(id);
        }
        inline typename const T &operator()( type id ) const {
            return get<component>(id);
        }
    };
    
    static inline
    std::string dump( const type &id ) {
        std::stringstream ss;
        for( auto &it : registered() ) {
            it->dump( ss, id );
        }
        return ss.str();
    }
    static inline
    type purge( const type &id ) { // clear
        for( auto &it : registered() ) {
            it->purge( id );
        }
        return id;
    }
    static inline
    type swap( const type &to, const type &from ) {
        for( auto &it : registered() ) {
            it->swap( to, from );
        }
        return to;
    }
    static inline
    type merge( type to, type from ) {
        for( auto &it : registered() ) {
            it->merge( to, from );
        }
        return to;
    }
    static inline
    type copy( type to, type from ) {
        for( auto &it : registered() ) {
            it->copy( to, from );
        }
        return to;
    }
    static inline
    type spawn( const type &from ) {
        return copy( id(), from );
    }
    static inline
    type reset( const type &id ) {
        return copy( id, none() );
    }
}

#endif
