# Translation progress function
function(generate_translation_progress)
    message(STATUS "=== Generating Translation Progress ===")
    
    set(TRANSLATION_FILES
        "${CMAKE_SOURCE_DIR}/i18n/QuickSoundSwitcher_en.ts"
        "${CMAKE_SOURCE_DIR}/i18n/QuickSoundSwitcher_fr.ts"
        "${CMAKE_SOURCE_DIR}/i18n/QuickSoundSwitcher_de.ts"
        "${CMAKE_SOURCE_DIR}/i18n/QuickSoundSwitcher_ko.ts"
        "${CMAKE_SOURCE_DIR}/i18n/QuickSoundSwitcher_it.ts"
        "${CMAKE_SOURCE_DIR}/i18n/QuickSoundSwitcher_zh_CN.ts"
    )

    set(PROGRESS_DATA "")
    
    foreach(TS_FILE ${TRANSLATION_FILES})
        get_filename_component(LANG ${TS_FILE} NAME_WE)
        string(REPLACE "QuickSoundSwitcher_" "" LANG_CODE "${LANG}")
        
        message(STATUS "Processing: ${TS_FILE}")
        message(STATUS "Language code: ${LANG_CODE}")
        
        if(EXISTS ${TS_FILE})
            file(READ ${TS_FILE} TS_CONTENT)
            
            # Count total messages
            string(REGEX MATCHALL "<message>" TOTAL_MATCHES ${TS_CONTENT})
            list(LENGTH TOTAL_MATCHES TOTAL_COUNT)
            
            # Count unfinished messages (those with type="unfinished")
            string(REGEX MATCHALL "<translation type=\"unfinished\"></translation>" UNFINISHED_MATCHES ${TS_CONTENT})
            list(LENGTH UNFINISHED_MATCHES UNFINISHED_COUNT)
            
            # Calculate finished count
            math(EXPR FINISHED_COUNT "${TOTAL_COUNT} - ${UNFINISHED_COUNT}")
            
            # Calculate percentage
            if(TOTAL_COUNT GREATER 0)
                math(EXPR PERCENTAGE "${FINISHED_COUNT} * 100 / ${TOTAL_COUNT}")
            else()
                set(PERCENTAGE 0)
            endif()
            
            message(STATUS "  Total: ${TOTAL_COUNT}")
            message(STATUS "  Unfinished: ${UNFINISHED_COUNT}")
            message(STATUS "  Finished: ${FINISHED_COUNT}")
            message(STATUS "  Percentage: ${PERCENTAGE}%")
            
            set(PROGRESS_DATA "${PROGRESS_DATA}        {\"${LANG_CODE}\", ${TOTAL_COUNT}, ${FINISHED_COUNT}, ${PERCENTAGE}},\n")
        else()
            message(WARNING "Translation file not found: ${TS_FILE}")
        endif()
    endforeach()

    message(STATUS "Generated progress data:")
    message(STATUS "${PROGRESS_DATA}")

    # Generate header file
    configure_file(
        "${CMAKE_SOURCE_DIR}/src/translation_progress.h.in"
        "${CMAKE_BINARY_DIR}/translation_progress.h"
        @ONLY
    )
    
    message(STATUS "Translation progress header generated at: ${CMAKE_BINARY_DIR}/translation_progress.h")
endfunction()