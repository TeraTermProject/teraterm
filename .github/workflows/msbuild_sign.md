
GitHub Actionsでビルド時の
バイナリを作成後
SignPath.io でコード署名の動作


```

GitHub                                     SignPath

----------------------------------------+-------------------------------------

- folder
  - installer/Output/portable
- contents
  - portable files (not ziped)

          |
          V

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
  - portable signd files (not ziped)

              |
              V

- actions/upload-artifact@v4
  - folder
    - installer/Output/portable_signed
  - contents
    - portable signd files (not ziped)
  - zip
    - portable_zip_signed.zip

              |
              V

- installer/iscc_signed.cmake
  - 未署名インストーラー作成
  - input folder
    - installer/Output/portable_signed
  - output folder / filename
    - installer/Output/setup/teraterm-unsigned.exe

              |
              V

- actions/upload-artifact@v4
  - folder
    - installer/Output/setup/
  - contents
    - teraterm-unsigned.exe (unsigned installer)
  - zip
    - setup_unsigned
                                       -->
                                              - Artifact Configurations
                                                - ziped_installer [XXX zipped]
                                       <--
- foler
  - installer/Output/portable_signed
- contents
  - teraterm-unsigned.exe (signed installer)

```
