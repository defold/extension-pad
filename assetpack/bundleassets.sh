# https://github.com/android/app-bundle-samples/tree/main/PlayAssetDelivery/BundletoolScriptSample

rm -rf assetpack_out
mkdir assetpack_out

./aapt2 link --proto-format --output-to-dir -o assetpack_out --manifest assetpack_in/manifest/AndroidManifest.xml -A assetpack_in/assets

rm assetpack_out/resources.pb
mkdir assetpack_out/manifest
mv assetpack_out/AndroidManifest.xml assetpack_out/manifest/AndroidManifest.xml

(cd assetpack_out && zip -r -0 assetpack.zip .)

java -jar bob.jar --platform=armv7-android --archive --bundle-module=assetpack_out/assetpack.zip --bundle-format=aab build bundle
# java -jar bundletool-all.jar build-bundle --modules assetpack_out/assetpack.zip --config bundleconfig.json --output assetpack.aab