// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

namespace embree
{ 
  /* 8-wide AVX-512 64bit long long type */
  template<>
    struct vllong<8>
  {
    typedef vboold8 Bool;

    enum  { size = 8 }; // number of SIMD elements
    union {             // data
      __m512i v; 
      long long i[8];
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////
       
    __forceinline vllong() {}
    __forceinline vllong(const vllong8& t) { v = t.v; }
    __forceinline vllong8& operator=(const vllong8& f) { v = f.v; return *this; }

    __forceinline vllong(const __m512i& t) { v = t; }
    __forceinline operator __m512i () const { return v; }
    __forceinline operator __m256i () const { return _mm512_castsi512_si256(v); }

    __forceinline vllong(const long long i) {
      v = _mm512_set1_epi64(i);
    }
    
    __forceinline vllong(const long long a, const long long b, const long long c, const long long d) {
      v = _mm512_set_4to8_epi64(d,c,b,a);      
    }

    __forceinline vllong(const long long a0, const long long a1, const long long a2, const long long a3,
                         const long long a4, const long long a5, const long long a6, const long long a7)
    {
      v = _mm512_set_8to8_epi64(a7,a6,a5,a4,a3,a2,a1,a0);
    }
   
    
    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////
    
    __forceinline vllong( ZeroTy   ) : v(_mm512_setzero_epi32()) {}
    __forceinline vllong( OneTy    ) : v(_mm512_set_1to8_epi64(1)) {}
    __forceinline vllong( StepTy   ) : v(_mm512_set_8to8_epi64(7,6,5,4,3,2,1,0)) {}
    __forceinline vllong( ReverseStepTy )   : v(_mm512_setr_epi64(7,6,5,4,3,2,1,0)) {}

    __forceinline static vllong8 zero() { return _mm512_setzero_epi32(); }
    __forceinline static vllong8 one () { return _mm512_set_1to8_epi64(1); }
    __forceinline static vllong8 neg_one () { return _mm512_set_1to8_epi64(-1); }

    ////////////////////////////////////////////////////////////////////////////////
    /// Loads and Stores
    ////////////////////////////////////////////////////////////////////////////////

    static __forceinline void store_nt(void *__restrict__ ptr, const vllong8& a) {
      _mm512_stream_si512(ptr,a);
    }

    static __forceinline vllong8 loadu(const void* addr)
    {
      return _mm512_loadu_si512(addr);
    }

    static __forceinline vllong8 load(const vllong8* addr) {
      return _mm512_load_si512(addr);
    }

    static __forceinline vllong8 load(const long long* addr) {
      return _mm512_load_si512(addr);
    }

    static __forceinline void store(void* ptr, const vllong8& v) {
      _mm512_store_si512(ptr,v);
    }

    static __forceinline void storeu(void* ptr, const vllong8& v ) {
      _mm512_storeu_si512(ptr,v);
    }

    static __forceinline void storeu(const vboold8& mask, long long* ptr, const vllong8& f ) {
      _mm512_mask_storeu_epi64(ptr,mask,f);
    }

    static __forceinline void store(const vboold8& mask, void* addr, const vllong8& v2) {
      _mm512_mask_store_epi64(addr,mask,v2);
    }

  /* pass by value to avoid compiler generating inefficient code */
    static __forceinline void storeu_compact(const vboold8 mask,void * addr, const vllong8& reg) {
      _mm512_mask_compressstoreu_epi64(addr,mask,reg);
    }

    static __forceinline vllong8 compact64bit(const vboold8& mask, vllong8& v) {
      return _mm512_mask_compress_epi64(v,mask,v);
    }

    static __forceinline vllong8 compact64bit(const vboold8& mask, vllong8& dest, const vllong8& source) {
      return _mm512_mask_compress_epi64(dest,mask,source);
    }

    static __forceinline vllong8 compact(const vboold8& mask, vllong8& v) {
      return _mm512_mask_compress_epi64(v,mask,v);
    }

    static __forceinline vllong8 compact(const vboold8& mask, const vllong8& a, vllong8& b) {
      return _mm512_mask_compress_epi64(a,mask,b);
    }

    static __forceinline vllong8 broadcast64bit(size_t v) {
      return _mm512_set1_epi64(v);
    }

    static __forceinline size_t extract64bit(const vllong8& v)
    {
      return _mm_cvtsi128_si64(_mm512_castsi512_si128(v));
    }


    ////////////////////////////////////////////////////////////////////////////////
    /// Array Access
    ////////////////////////////////////////////////////////////////////////////////
    
    __forceinline       long long& operator[](const size_t index)       { assert(index < 8); return i[index]; }
    __forceinline const long long& operator[](const size_t index) const { assert(index < 8); return i[index]; }

  };
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline const vllong8 asLong    ( const __m512& a ) { return _mm512_castps_si512(a); }
  __forceinline const vllong8 operator +( const vllong8& a ) { return a; }
  __forceinline const vllong8 operator -( const vllong8& a ) { return _mm512_sub_epi64(_mm512_setzero_epi32(), a); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline const vllong8 operator +( const vllong8&  a, const vllong8&  b ) { return _mm512_add_epi64(a, b); }
  __forceinline const vllong8 operator +( const vllong8&  a, const long long b ) { return a + vllong8(b); }
  __forceinline const vllong8 operator +( const long long a, const vllong8&  b ) { return vllong8(a) + b; }

  __forceinline const vllong8 operator -( const vllong8&  a, const vllong8&  b ) { return _mm512_sub_epi64(a, b); }
  __forceinline const vllong8 operator -( const vllong8&  a, const long long b ) { return a - vllong8(b); }
  __forceinline const vllong8 operator -( const long long a, const vllong8&  b ) { return vllong8(a) - b; }

  __forceinline const vllong8 operator *( const vllong8&  a, const vllong8&  b ) { return _mm512_mullo_epi64(a, b); }
  __forceinline const vllong8 operator *( const vllong8&  a, const long long b ) { return a * vllong8(b); }
  __forceinline const vllong8 operator *( const long long a, const vllong8&  b ) { return vllong8(a) * b; }

  __forceinline const vllong8 operator &( const vllong8&  a, const vllong8&  b ) { return _mm512_and_epi64(a, b); }
  __forceinline const vllong8 operator &( const vllong8&  a, const long long b ) { return a & vllong8(b); }
  __forceinline const vllong8 operator &( const long long a, const vllong8&  b ) { return vllong8(a) & b; }

  __forceinline const vllong8 operator |( const vllong8&  a, const vllong8&  b ) { return _mm512_or_epi64(a, b); }
  __forceinline const vllong8 operator |( const vllong8&  a, const long long b ) { return a | vllong8(b); }
  __forceinline const vllong8 operator |( const long long a, const vllong8&  b ) { return vllong8(a) | b; }

  __forceinline const vllong8 operator ^( const vllong8&  a, const vllong8&  b ) { return _mm512_xor_epi64(a, b); }
  __forceinline const vllong8 operator ^( const vllong8&  a, const long long b ) { return a ^ vllong8(b); }
  __forceinline const vllong8 operator ^( const long long a, const vllong8&  b ) { return vllong8(a) ^ b; }

  __forceinline const vllong8 operator <<( const vllong8& a, const long long n ) { return _mm512_slli_epi64(a, n); }
  __forceinline const vllong8 operator >>( const vllong8& a, const long long n ) { return _mm512_srai_epi64(a, n); }

  __forceinline const vllong8 operator <<( const vllong8& a, const vllong8& n ) { return _mm512_sllv_epi64(a, n); }
  __forceinline const vllong8 operator >>( const vllong8& a, const vllong8& n ) { return _mm512_srav_epi64(a, n); }

  __forceinline const vllong8 sll ( const vllong8& a, const long long b ) { return _mm512_slli_epi64(a, b); }
  __forceinline const vllong8 sra ( const vllong8& a, const long long b ) { return _mm512_srai_epi64(a, b); }
  __forceinline const vllong8 srl ( const vllong8& a, const long long b ) { return _mm512_srli_epi64(a, b); }
  
  __forceinline const vllong8 min( const vllong8&  a, const vllong8&  b ) { return _mm512_min_epi64(a, b); }
  __forceinline const vllong8 min( const vllong8&  a, const long long b ) { return min(a,vllong8(b)); }
  __forceinline const vllong8 min( const long long a, const vllong8&  b ) { return min(vllong8(a),b); }

  __forceinline const vllong8 max( const vllong8&  a, const vllong8&  b ) { return _mm512_max_epi64(a, b); }
  __forceinline const vllong8 max( const vllong8&  a, const long long b ) { return max(a,vllong8(b)); }
  __forceinline const vllong8 max( const long long a, const vllong8&  b ) { return max(vllong8(a),b); }
  
  __forceinline const vllong8 mask_add(const vboold8& m, const vllong8& c, const vllong8& a, const vllong8& b) { return _mm512_mask_add_epi64(c,m,a,b); }
  __forceinline const vllong8 mask_sub(const vboold8& m, const vllong8& c, const vllong8& a, const vllong8& b) { return _mm512_mask_sub_epi64(c,m,a,b); }

  __forceinline const vllong8 mask_and(const vboold8& m, const vllong8& c, const vllong8& a, const vllong8& b) { return _mm512_mask_and_epi64(c,m,a,b); }
  __forceinline const vllong8 mask_or (const vboold8& m, const vllong8& c, const vllong8& a, const vllong8& b) { return _mm512_mask_or_epi64(c,m,a,b); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Assignment Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline vllong8& operator +=( vllong8& a, const vllong8&  b ) { return a = a + b; }
  __forceinline vllong8& operator +=( vllong8& a, const long long b ) { return a = a + b; }
  
  __forceinline vllong8& operator -=( vllong8& a, const vllong8&  b ) { return a = a - b; }
  __forceinline vllong8& operator -=( vllong8& a, const long long b ) { return a = a - b; }

  __forceinline vllong8& operator *=( vllong8& a, const vllong8&  b ) { return a = a * b; }
  __forceinline vllong8& operator *=( vllong8& a, const long long b ) { return a = a * b; }
  
  __forceinline vllong8& operator &=( vllong8& a, const vllong8&  b ) { return a = a & b; }
  __forceinline vllong8& operator &=( vllong8& a, const long long b ) { return a = a & b; }
  
  __forceinline vllong8& operator |=( vllong8& a, const vllong8&  b ) { return a = a | b; }
  __forceinline vllong8& operator |=( vllong8& a, const long long b ) { return a = a | b; }
  
  __forceinline vllong8& operator <<=( vllong8& a, const long long b ) { return a = a << b; }
  __forceinline vllong8& operator >>=( vllong8& a, const long long b ) { return a = a >> b; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators + Select
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline const vboold8 operator ==( const vllong8&  a, const vllong8&  b ) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_EQ); }
  __forceinline const vboold8 operator ==( const vllong8&  a, const long long b ) { return a == vllong8(b); }
  __forceinline const vboold8 operator ==( const long long a, const vllong8&  b ) { return vllong8(a) == b; }
  
  __forceinline const vboold8 operator !=( const vllong8&  a, const vllong8&  b ) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_NE); }
  __forceinline const vboold8 operator !=( const vllong8&  a, const long long b ) { return a != vllong8(b); }
  __forceinline const vboold8 operator !=( const long long a, const vllong8&  b ) { return vllong8(a) != b; }
  
  __forceinline const vboold8 operator < ( const vllong8&  a, const vllong8&  b ) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_LT); }
  __forceinline const vboold8 operator < ( const vllong8&  a, const long long b ) { return a <  vllong8(b); }
  __forceinline const vboold8 operator < ( const long long a, const vllong8&  b ) { return vllong8(a) <  b; }
  
  __forceinline const vboold8 operator >=( const vllong8&  a, const vllong8&  b ) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_GE); }
  __forceinline const vboold8 operator >=( const vllong8&  a, const long long b ) { return a >= vllong8(b); }
  __forceinline const vboold8 operator >=( const long long a, const vllong8&  b ) { return vllong8(a) >= b; }

  __forceinline const vboold8 operator > ( const vllong8&  a, const vllong8&  b ) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_GT); }
  __forceinline const vboold8 operator > ( const vllong8&  a, const long long b ) { return a >  vllong8(b); }
  __forceinline const vboold8 operator > ( const long long a, const vllong8&  b ) { return vllong8(a) >  b; }

  __forceinline const vboold8 operator <=( const vllong8&  a, const vllong8&  b ) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_LE); }
  __forceinline const vboold8 operator <=( const vllong8&  a, const long long b ) { return a <= vllong8(b); }
  __forceinline const vboold8 operator <=( const long long a, const vllong8&  b ) { return vllong8(a) <= b; }

  __forceinline vboold8 eq(const vllong8& a, const vllong8& b) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_EQ); }
  __forceinline vboold8 ne(const vllong8& a, const vllong8& b) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_NE); }
  __forceinline vboold8 lt(const vllong8& a, const vllong8& b) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_LT); }
  __forceinline vboold8 ge(const vllong8& a, const vllong8& b) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_GE); }
  __forceinline vboold8 gt(const vllong8& a, const vllong8& b) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_GT); }
  __forceinline vboold8 le(const vllong8& a, const vllong8& b) { return _mm512_cmp_epi64_mask(a,b,_MM_CMPINT_LE); }
    
  __forceinline vboold8 eq(const vboold8 mask, const vllong8& a, const vllong8& b) { return _mm512_mask_cmp_epi64_mask(mask,a,b,_MM_CMPINT_EQ); }
  __forceinline vboold8 ne(const vboold8 mask, const vllong8& a, const vllong8& b) { return _mm512_mask_cmp_epi64_mask(mask,a,b,_MM_CMPINT_NE); }
  __forceinline vboold8 lt(const vboold8 mask, const vllong8& a, const vllong8& b) { return _mm512_mask_cmp_epi64_mask(mask,a,b,_MM_CMPINT_LT); }
  __forceinline vboold8 ge(const vboold8 mask, const vllong8& a, const vllong8& b) { return _mm512_mask_cmp_epi64_mask(mask,a,b,_MM_CMPINT_GE); }
  __forceinline vboold8 gt(const vboold8 mask, const vllong8& a, const vllong8& b) { return _mm512_mask_cmp_epi64_mask(mask,a,b,_MM_CMPINT_GT); }
  __forceinline vboold8 le(const vboold8 mask, const vllong8& a, const vllong8& b) { return _mm512_mask_cmp_epi64_mask(mask,a,b,_MM_CMPINT_LE); }

  __forceinline const vllong8 select( const vboold8& m, const vllong8& t, const vllong8& f ) {
    return _mm512_mask_or_epi64(f,m,t,t); 
  }

  __forceinline void xchg(const vboold8& m, vllong8& a, vllong8& b) {
    const vllong8 c = a; a = select(m,b,a); b = select(m,c,b);
  }

  __forceinline vboold8 test(const vboold8& m, const vllong8& a, const vllong8& b) {
    return _mm512_mask_test_epi64_mask(m,a,b);
  }

  __forceinline vboold8 test(const vllong8& a, const vllong8& b) {
    return _mm512_test_epi64_mask(a,b);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline vllong8 shuffle (const vllong8& x,int perm32 ) { return _mm512_permutex_epi64(x,perm32); }
  __forceinline vllong8 shuffle4(const vllong8& x,int perm128) { return _mm512_shuffle_i64x2(x,x,perm128); }
  
  template<int D, int C, int B, int A> __forceinline vllong8 shuffle   (const vllong8& v) { return _mm512_permutex_epi64(v,(int)_MM_SHUF_PERM(D,C,B,A)); }
  template<int A>                      __forceinline vllong8 shuffle   (const vllong8& x) { return shuffle<A,A,A,A>(v); }

  template<int D, int C, int B, int A> __forceinline vllong8 shuffle4(const vllong8& v) { return shuffle4(v,(int)_MM_SHUF_PERM(D,C,B,A)); }
  template<int A>                      __forceinline vllong8 shuffle4(const vllong8& x) { return shuffle4<A,A,A,A>(x); }

  template<int i>
    __forceinline vllong8 align_shift_right(const vllong8& a, const vllong8& b)
  {
    return _mm512_alignr_epi64(a,b,i); 
  };

  __forceinline long long toScalar(const vllong8& a)
  {
    return _mm_cvtsi128_si64(_mm512_castsi512_si128(a));
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Reductions
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline long long reduce_add(const vllong8& a) { return _mm512_reduce_add_epi64(a); }
  __forceinline long long reduce_min(const vllong8& a) { return _mm512_reduce_min_epi64(a); }
  __forceinline long long reduce_max(const vllong8& a) { return _mm512_reduce_max_epi64(a); }
  __forceinline long long reduce_and(const vllong8& a) { return _mm512_reduce_and_epi64(a); }
  
  __forceinline vllong8 vreduce_min2(const vllong8& x) {                                   return min(x,shuffle(x,_MM_SHUF_PERM(2,3,0,1))); }
  __forceinline vllong8 vreduce_min4(const vllong8& y) { const vllong8 x = vreduce_min2(y); return min(x,shuffle(x,_MM_SHUF_PERM(1,0,3,2))); }
  __forceinline vllong8 vreduce_min (const vllong8& y) { const vllong8 x = vreduce_min4(y); return min(x,shuffle4(x,_MM_SHUF_PERM(1,0,3,2))); }

  __forceinline vllong8 vreduce_max2(const vllong8& x) {                                   return max(x,shuffle(x,_MM_SHUF_PERM(1,0,3,2))); }
  __forceinline vllong8 vreduce_max4(const vllong8& y) { const vllong8 x = vreduce_max2(y); return max(x,shuffle(x,_MM_SHUF_PERM(2,3,0,1))); }
  __forceinline vllong8 vreduce_max (const vllong8& y) { const vllong8 x = vreduce_max4(y); return max(x,shuffle4(x,_MM_SHUF_PERM(1,0,3,2))); }

  __forceinline vllong8 vreduce_and2(const vllong8& x) {                                   return x & shuffle(x,_MM_SHUF_PERM(1,0,3,2)); }
  __forceinline vllong8 vreduce_and4(const vllong8& y) { const vllong8 x = vreduce_and2(y); return x & shuffle(x,_MM_SHUF_PERM(2,3,0,1)); }
  __forceinline vllong8 vreduce_and (const vllong8& y) { const vllong8 x = vreduce_and4(y); return x & shuffle4(x,_MM_SHUF_PERM(1,0,3,2)); }

  __forceinline vllong8 vreduce_or2(const vllong8& x) {                                  return x | shuffle(x,_MM_SHUF_PERM(1,0,3,2)); }
  __forceinline vllong8 vreduce_or4(const vllong8& y) { const vllong8 x = vreduce_or2(y); return x | shuffle(x,_MM_SHUF_PERM(2,3,0,1)); }
  __forceinline vllong8 vreduce_or (const vllong8& y) { const vllong8 x = vreduce_or4(y); return x | shuffle4(x,_MM_SHUF_PERM(1,0,3,2)); }

  __forceinline vllong8 vreduce_add2(const vllong8& x) {                                   return x + shuffle(x,_MM_SHUF_PERM(1,0,3,2)); }
  __forceinline vllong8 vreduce_add4(const vllong8& y) { const vllong8 x = vreduce_add2(y); return x + shuffle(x,_MM_SHUF_PERM(2,3,0,1)); }
  __forceinline vllong8 vreduce_add (const vllong8& y) { const vllong8 x = vreduce_add4(y); return x + shuffle4(x,_MM_SHUF_PERM(1,0,3,2)); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Memory load and store operations
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline vllong8 permute(const vllong8& v, const vllong8& index) {
    return _mm512_permutexvar_epi64(index,v);  
  }

  __forceinline vllong8 reverse(const vllong8& a) {
    return permute(a,vllong8(reverse_step));
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////
  
  __forceinline std::ostream& operator<<(std::ostream& cout, const vllong8& v)
  {
    cout << "<" << v[0];
    for (size_t i=1; i<8; i++) cout << ", " << v[i];
    cout << ">";
    return cout;
  }
}
