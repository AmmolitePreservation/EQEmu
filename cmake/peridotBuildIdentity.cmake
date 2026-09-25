EXECUTE_PROCESS(
  COMMAND git -C "${sourceDirectory}" rev-parse HEAD
  OUTPUT_VARIABLE gitCommit
  RESULT_VARIABLE revParseResult
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
IF(NOT revParseResult EQUAL 0)
  MESSAGE(FATAL_ERROR "Peridot build identity: git rev-parse failed in ${sourceDirectory}")
ENDIF()

EXECUTE_PROCESS(
  COMMAND git -C "${sourceDirectory}" status --porcelain --untracked-files=no --ignore-submodules=dirty
  OUTPUT_VARIABLE gitChanges
  RESULT_VARIABLE statusResult
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
IF(NOT statusResult EQUAL 0)
  MESSAGE(FATAL_ERROR "Peridot build identity: git status failed in ${sourceDirectory}")
ENDIF()

IF(gitChanges STREQUAL "")
  SET(gitDirty "false")
ELSE()
  SET(gitDirty "true")
ENDIF()

SET(identityHeader "#pragma once\n\nconstexpr const char *peridotGitCommit = \"${gitCommit}\";\nconstexpr bool peridotGitDirty = ${gitDirty};\n")

# Rewritten only when the value changes, so an unchanged commit triggers no recompiles.
SET(existingHeader "")
IF(EXISTS "${outputFile}")
  FILE(READ "${outputFile}" existingHeader)
ENDIF()
IF(NOT existingHeader STREQUAL identityHeader)
  FILE(WRITE "${outputFile}" "${identityHeader}")
ENDIF()
