# Copyright (c) 2013 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Global variables
# CALLIGRA_SHOULD_BUILD_PRODUCTS - list of requested products by the user
# CALLIGRA_NEEDED_PRODUCTS - list of internal needed products
# CALLIGRA_WANTED_PRODUCTS - list of internal wanted products
# CALLIGRA_STAGING_PRODUCTS - list of products only in staging mode
# SHOULD_BUILD_${product_id} - boolean if product should be build


macro(calligra_disable_product _product_id _reason)
  set(SHOULD_BUILD_${_product_id} FALSE)
  if (NOT BUILD_${_product_id}_DISABLE_REASON)
    set(BUILD_${_product_id}_DISABLE_REASON "${_reason}")
  endif (NOT BUILD_${_product_id}_DISABLE_REASON)
endmacro()

macro(calligra_set_shouldbuild_dependentproduct _product_id _dep_product_id)
  # if not already enabled, enable
  if (NOT SHOULD_BUILD_${_dep_product_id})
    list(APPEND CALLIGRA_NEEDED_PRODUCTS ${_dep_product_id})
    set(SHOULD_BUILD_${_dep_product_id} TRUE)
    if (DEFINED CALLIGRA_PRODUCT_${_dep_product_id}_needed_dependencies OR
        DEFINED CALLIGRA_PRODUCT_${_dep_product_id}_wanted_dependencies)
        calligra_set_shouldbuild_productdependencies(${_dep_product_id}
            "${CALLIGRA_PRODUCT_${_dep_product_id}_needed_dependencies}"
            "${CALLIGRA_PRODUCT_${_dep_product_id}_wanted_dependencies}")
    endif (DEFINED CALLIGRA_PRODUCT_${_dep_product_id}_needed_dependencies OR
           DEFINED CALLIGRA_PRODUCT_${_dep_product_id}_wanted_dependencies)
  endif (NOT SHOULD_BUILD_${_dep_product_id})
endmacro()


macro(calligra_set_shouldbuild_productdependencies _product_id _productset_id_needed_dependencies  _productset_id_wanted_dependencies)
  # activate all needed products and note the dependency
  foreach(_dep_product_id ${_productset_id_needed_dependencies})
    list(APPEND CALLIGRA_PRODUCT_${_dep_product_id}_dependents ${_product_id})
    calligra_set_shouldbuild_dependentproduct(${_product_id} ${_dep_product_id})
  endforeach(_dep_product_id)

  # activate all wanted products
  foreach(_dep_product_id ${_productset_id_wanted_dependencies})
    calligra_set_shouldbuild_dependentproduct(${_product_id} ${_dep_product_id})
  endforeach(_dep_product_id)
endmacro()


macro(calligra_drop_unbuildable_products)
  # first drop all staging products if not in debug build
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "DebugFull")
    foreach(_product_id ${CALLIGRA_STAGING_PRODUCTS})
      calligra_disable_product(${_product_id} "Not ready for release")
    endforeach(_product_id)
  endif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "DebugFull")

  # can assume calligra_all_products has products in down-up order
  # 1. check all wanted products and see if they will be built,
  #    if not then drop their required products
  # TODO!
  # 2. check all products if they can be built, if not disable build of depending
  foreach(_product_id ${CALLIGRA_ALL_PRODUCTS})
    if(NOT SHOULD_BUILD_${_product_id})
      if(DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)
        foreach(_dependent_product_id ${CALLIGRA_PRODUCT_${_product_id}_dependents})
          calligra_disable_product(${_dependent_product_id} "Required internal dependency missing: ${_product_id}")
        endforeach(_dependent_product_id ${CALLIGRA_PRODUCT_${_dep_product_id}_dependents})
      endif(DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)
    endif(NOT SHOULD_BUILD_${_product_id})
  endforeach(_product_id)
endmacro()

# backward compatibility: BUILD_app as option or passed as cmake parameter can overwrite product set
# and disable a product if set to FALSE (TRUE was ignored)
macro(calligra_drop_products_on_old_flag _build_id)
  string(TOLOWER "${_build_id}" lowercase_build_id)
  if (DEFINED BUILD_${lowercase_build_id} AND NOT BUILD_${lowercase_build_id})
    foreach(_product_id ${ARGN})
      if (NOT DEFINED SHOULD_BUILD_${_product_id})
        message(FATAL_ERROR "Unknown product: ${_product_id}")
      endif (NOT DEFINED SHOULD_BUILD_${_product_id})
      calligra_disable_product(${_product_id} "Disabled by cmake parameter")
    endforeach(_product_id)
  endif (DEFINED BUILD_${lowercase_build_id} AND NOT BUILD_${lowercase_build_id})
endmacro()

macro(calligra_set_productset _productset_id)
  # be gracefull and compare the productset id case insensitive, by lowercasing
  # also expects the productset definition filename in all lowercase
  string(TOLOWER "${_productset_id}" lowercase_productset_id)
  set(productset_filename "${CMAKE_SOURCE_DIR}/cmake/productsets/${lowercase_productset_id}.cmake")

  if (NOT EXISTS "${productset_filename}")
     message(FATAL_ERROR "Unknown product set: ${_productset_id}")
  endif (NOT EXISTS "${productset_filename}")

  # include the productset definition
  include(${productset_filename})
  if (NOT DEFINED CALLIGRA_SHOULD_BUILD_PRODUCTS)
    message(FATAL_ERROR "Product set file \"${productset_filename}\" did not define the var CALLIGRA_SHOULD_BUILD_PRODUCTS.")
  endif (NOT DEFINED CALLIGRA_SHOULD_BUILD_PRODUCTS)
  set(CALLIGRA_NEEDED_PRODUCTS "")

  message(STATUS "--------------------------------------------------------------------------" )
  message(STATUS "Configured with product set \"${_productset_id}\"")
  message(STATUS "--------------------------------------------------------------------------" )

  # backward compatibility: BUILD_app as option or passed as cmake parameter can overwrite product set
  # and disable a product if set to FALSE (TRUE was ignored)
  foreach(_product_id ${CALLIGRA_ALL_PRODUCTS})
    string(TOLOWER "${_product_id}" lowercase_product_id)
    if (DEFINED BUILD_${lowercase_product_id})
      if(NOT BUILD_${lowercase_product_id})
        list(FIND CALLIGRA_SHOULD_BUILD_PRODUCTS ${_product_id} _index)
        # remove from product set, if part
        if(NOT _index EQUAL -1)
          list(REMOVE_AT CALLIGRA_SHOULD_BUILD_PRODUCTS ${_index})
        endif(NOT _index EQUAL -1)
      endif(NOT BUILD_${lowercase_product_id})
    endif (DEFINED BUILD_${lowercase_product_id})
  endforeach(_product_id ${CALLIGRA_ALL_PRODUCTS})

  # mark all products of the set as SHOULD_BUILD
  foreach(_product_id ${CALLIGRA_SHOULD_BUILD_PRODUCTS})
    # check that this product is actually existing
    if (NOT DEFINED SHOULD_BUILD_${_product_id})
      message(FATAL_ERROR "Unknown product: ${_product_id}")
    endif (NOT DEFINED SHOULD_BUILD_${_product_id})

    # mark product as should build, also all dependencies
    set(SHOULD_BUILD_${_product_id} TRUE)
    if (DEFINED CALLIGRA_PRODUCT_${_product_id}_needed_dependencies OR
        DEFINED CALLIGRA_PRODUCT_${_product_id}_wanted_dependencies)
        calligra_set_shouldbuild_productdependencies(${_product_id}
            "${CALLIGRA_PRODUCT_${_product_id}_needed_dependencies}"
            "${CALLIGRA_PRODUCT_${_product_id}_wanted_dependencies}")
    endif (DEFINED CALLIGRA_PRODUCT_${_product_id}_needed_dependencies OR
           DEFINED CALLIGRA_PRODUCT_${_product_id}_wanted_dependencies)
  endforeach(_product_id)
endmacro()


# Usage:
#   calligra_define_product(<product_id>
#         [NAME] <product_name>
#         [STAGING]
#         [NEEDS <product_id1> [<product_id2> ...]]
#         [WANTS <product_id1> [<product_id2> ...]]
#       )
macro(calligra_define_product _product_id)
  # default product name to id, empty deps
  set(_product_name "${_product_id}")
  set(_needed_dep_product_ids)
  set(_wanted_dep_product_ids)

  # parse arguments: three states, "name", "needs" or "wants"
  set(_current_arg_type "name")
  foreach(_arg ${ARGN})
    if(${_arg} STREQUAL "NAME")
      set(_current_arg_type "name")
    elseif(${_arg} STREQUAL "NEEDS")
      set(_current_arg_type "needs")
    elseif(${_arg} STREQUAL "WANTS")
      set(_current_arg_type "wants")
    else(${_arg} STREQUAL "NAME")
      if(${_current_arg_type} STREQUAL "name")
        if(${_arg} STREQUAL "STAGING")
          list(APPEND CALLIGRA_STAGING_PRODUCTS ${_product_id})
        else(${_arg} STREQUAL "STAGING")
          set(_product_name "${_arg}")
        endif(${_arg} STREQUAL "STAGING")
      elseif(${_current_arg_type} STREQUAL "needs")
        # check that the dependency is actually existing
        if(NOT DEFINED SHOULD_BUILD_${_arg})
          message(FATAL_ERROR "Unknown product listed as dependency for ${_product_id}: ${_arg}")
        endif(NOT DEFINED SHOULD_BUILD_${_arg})
        list(APPEND _needed_dep_product_ids "${_arg}")
      elseif(${_current_arg_type} STREQUAL "wants")
        # check that the dependency is actually existing
        if(NOT DEFINED SHOULD_BUILD_${_arg})
          message(FATAL_ERROR "Unknown product listed as dependency for ${_product_id}: ${_arg}")
        endif(NOT DEFINED SHOULD_BUILD_${_arg})
        list(APPEND _wanted_dep_product_ids "${_arg}")
      endif(${_current_arg_type} STREQUAL "name")
    endif(${_arg} STREQUAL "NAME")
  endforeach(_arg)

  # set product vars
  set(SHOULD_BUILD_${_product_id} FALSE)
  set(CALLIGRA_PRODUCT_${_product_id}_name "${_product_name}")
  set(CALLIGRA_PRODUCT_${_product_id}_needed_dependencies ${_needed_dep_product_ids})
  set(CALLIGRA_PRODUCT_${_product_id}_wanted_dependencies ${_wanted_dep_product_ids})
  list(APPEND CALLIGRA_ALL_PRODUCTS ${_product_id})
endmacro(calligra_define_product)


macro(calligra_log_should_build)
  # find what products will be built and which not
  set(_built_product_ids "")
  set(_not_built_product_ids "")
  set(_built_dependency_product_ids "")
  set(_not_built_dependency_product_ids "")

  foreach(_product_id ${CALLIGRA_ALL_PRODUCTS})
    list(FIND CALLIGRA_SHOULD_BUILD_PRODUCTS ${_product_id} _index)
    if(NOT _index EQUAL -1)
      if(SHOULD_BUILD_${_product_id})
        list(APPEND _built_product_ids ${_product_id})
      else(SHOULD_BUILD_${_product_id})
        list(APPEND _not_built_product_ids ${_product_id})
      endif(SHOULD_BUILD_${_product_id})
    else(NOT _index EQUAL -1)
      if(SHOULD_BUILD_${_product_id})
        list(APPEND _built_dependency_product_ids ${_product_id})
      else(SHOULD_BUILD_${_product_id})
        list(FIND CALLIGRA_NEEDED_PRODUCTS ${_product_id} _index2)
        if(NOT _index2 EQUAL -1)
          list(APPEND _not_built_dependency_product_ids ${_product_id})
        endif(NOT _index2 EQUAL -1)
      endif(SHOULD_BUILD_${_product_id})
    endif(NOT _index EQUAL -1)
  endforeach(_product_id)

  if(NOT _built_dependency_product_ids STREQUAL "")
    message(STATUS "---------------- The following required products will be built ---------" )
    foreach(_product_id ${_built_dependency_product_ids})
      if (DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)
        set(dependents "   [[needed by: ${CALLIGRA_PRODUCT_${_product_id}_dependents}]]")
      else (DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)
        set(dependents "")
      endif (DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)

      message(STATUS "${_product_id}:  ${CALLIGRA_PRODUCT_${_product_id}_name}${dependents}" )
    endforeach(_product_id)
    message(STATUS "")
  endif(NOT _built_dependency_product_ids STREQUAL "")
  if(NOT _not_built_dependency_product_ids STREQUAL "")
    message(STATUS "---------------- The following required products can NOT be built -----" )
    foreach(_product_id ${_not_built_dependency_product_ids})
      if (DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)
        set(dependents "   [[needed by: ${CALLIGRA_PRODUCT_${_product_id}_dependents}]]")
      else (DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)
        set(dependents "")
      endif (DEFINED CALLIGRA_PRODUCT_${_product_id}_dependents)

      message(STATUS "${_product_id}:  ${CALLIGRA_PRODUCT_${_product_id}_name}${dependents}  |  ${BUILD_${_product_id}_DISABLE_REASON}" )
    endforeach(_product_id)
    message(STATUS "")
  endif(NOT _not_built_dependency_product_ids STREQUAL "")
  message(STATUS "---------------- The following products will be built --------------------" )
  foreach(_product_id ${_built_product_ids})
      message(STATUS "${_product_id}:  ${CALLIGRA_PRODUCT_${_product_id}_name}" )
  endforeach(_product_id)
  if(NOT _not_built_product_ids STREQUAL "")
    message(STATUS "\n---------------- The following products can NOT be built ------------" )
    foreach(_product_id ${_not_built_product_ids})
        message(STATUS "${_product_id}:  ${CALLIGRA_PRODUCT_${_product_id}_name}  |  ${BUILD_${_product_id}_DISABLE_REASON}" )
    endforeach(_product_id)
  endif(NOT _not_built_product_ids STREQUAL "")
  message(STATUS "--------------------------------------------------------------------------" )
endmacro(calligra_log_should_build)
