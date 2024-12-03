# Usage:
#   source activate.sh
export HERE=`dirname "$(readlink -f "$BASH_SOURCE")"`
IDFDIR=${HOME}/esp/idf


source /usr/local/lib/shellenv.sh
shellenv_init esp32 $BASH_SOURCE
# shellenv_set PROJECT_NAME FOO

_PATHBACK=${PATH}
source ${IDFDIR}/export.sh


shellenv_before_deactivate() {
  echo -n "Deactivating ${ENV_TITLE} ... "
  unset IDF_PYTHON_ENV_PATH
  unset IDF_PATH
  unset OPENOCD_SCRIPTS
  unset ESP_IDF_VERSION
  unset IDF_DEACTIVATE_FILE_PATH
  unset IDF_TOOLS_EXPORT_CMD
  unset IDF_TOOLS_INSTALL_CMD
  unset ESP_ROM_ELF_DIR
}


shellenv_after_deactivate() {
  echo "success."
  export PATH=${_PATHBACK}
  unset _PATHBACK
}
