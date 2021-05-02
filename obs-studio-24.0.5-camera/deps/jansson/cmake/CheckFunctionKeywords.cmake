include(CheckCSourceCompiles)

macro(check_function_keywords _wordlist)
  set(${_result} "")
  foreach(flag ${_wordlist})
    string(REGEX REPLACE "[-+/ ()]" "_" flagname "${flag}")
    string(TOUPPER "${flagname}" flagname)
    set(have_flag "HAVE_${flagname}")
    check_c_source_compiles("${flag} void func(); void func() { } int main() { func(); return 0; }" ${have_flag})
    if(${have_flag} AND NOT ${_result})
      set(${_result} "${flag}")
#      break()
    endif(${have_flag} AND NOT ${_result})
  endforeach(flag)
endmacro(check_function_keywords)
