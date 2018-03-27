mkdir knowbug
xcopy package /S /-Y knowbug /EXCLUDE:package_exclude.txt
copy changes.md knowbug\changes.md
copy README.md  knowbug\README.md
copy src\Release\hsp3debug.dll knowbug\hsp3debug_knowbug.dll
copy src\x64\Release2\hsp3debug_64.dll knowbug\hsp3debug_64_knowbug.dll
copy package\sample\VswFizzBuzz\Release\VswFizzBuzz.dll knowbug\sample\VswFizzBuzz\VswFizzBuzz.dll
7za a knowbug-package.zip knowbug
rmdir /S knowbug
