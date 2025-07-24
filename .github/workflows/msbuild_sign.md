
GitHub Actionsでビルド時の
バイナリを作成後
SignPath.io でコード署名の動作


```

GitHub                                     SignPath

----------------------------------------+-------------------------------------

- name: build_scirpt
  - script makearchive.bat
- folder
  - installer/teraterm
  - installer/teraterm_pdb
- contents
  - portable files (not zipped)


- name: build_scirpt
  - script iscc.bat
- folder
  - installer/Output/portable/teraterm
- contents
  - portable files (not zipped)


- name: Upload artifacts for sign (tareterm_portable.zip)
- folder
  - installer/Output/portable
- contents
  - portable files (not zipped)

          |
          V

- name: sign portable zip
  - actions/upload-artifact@v4
  - folder
    - installer/Output/portable
  - zip
    - portable_unsigned.zip
                                       -->
                                              - Artifact Configurations
                                                - teraterm_portable
                                       <--
  - foler
    - installer/Output/portable_signed
  - contents
   - portable signd files (not zipped)

              |
              V

- name: rename portable signed zip
- actions/upload-artifact@v4
  - folder
    - installer/Output/portable_signed
  - contents
    - portable signd files (not zipped)
  - zip
    - portable_zip_signed.zip

              |
              V

- name: unsigned setup
  - installer/iscc_signed.cmake
  - 未署名インストーラー作成
  - input folder
    - installer/Output/portable_signed
  - output folder / filename
    - installer/Output/setup/teraterm-unsigned.exe

              |
              V

- name: Upload artifacts for sign (setup.exe)
  - actions/upload-artifact@v4
  - folder
    - installer/Output/setup/
  - contents
    - teraterm-unsigned.exe (unsigned installer)
  - zip
    - setup_unsigned
                                       -->
                                              - Artifact Configurations
                                                - zipped_installer [XXX zipped]
                                       <--
  - foler
    - installer/Output/portable_signed
  - contents
    - teraterm-unsigned.exe (signed installer)

```
