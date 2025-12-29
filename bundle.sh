set -e

APP_NAME=PlayAssetDelivery
ASSETPACKS=$(realpath assetpacks)
BOB=$(realpath bob.jar)
VARIANT=debug
LOCAL_TESTING=true


# make build
rm -rf bundle
mkdir bundle
BUNDLE_DIR=$(realpath bundle)

MAIN_AAB=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.aab

# build .aab file
# use liveupdate to get excluded content as a zip file (see liveupdate.settings)
echo "---- BUILD MAIN AAB"
java -jar ${BOB} --defoldsdk=1ba9e1aa422166864c3267f03f5110144b745c1e --platform=armv7-android --archive --variant=${VARIANT} --liveupdate=yes --bundle-output=${BUNDLE_DIR} --bundle-format=aab build bundle


# create asset packs
echo "---- CREATE ASSET PACKS"
pushd ${ASSETPACKS}
gradle assetPackDebugPreBundleTask

rm -rf ${ASSETPACKS}/out
mkdir ${ASSETPACKS}/out
for PACK_NAME in asset_pack_1 asset_pack_2
do
	OUT_DIR=${ASSETPACKS}/out/${PACK_NAME}

	echo "---- ADD ASSET PACK TO MAIN AAB"
	rm -rf ${OUT_DIR}
	mkdir ${OUT_DIR}
	unzip build/intermediates/asset_pack_bundle/debug/assetPackDebugPreBundleTask/${PACK_NAME}/${PACK_NAME}.zip -d ${OUT_DIR}
	pushd ${ASSETPACKS}/out
	# -D do not write directory entries to the archive
	zip -r -0 -D ${MAIN_AAB} ${PACK_NAME}
	popd
done
popd

# sign .aab
echo "---- SIGN AAB"
java -cp ${BOB} com.dynamo.bob.tools.AndroidTools jarsigner -verbose -keystore debug.keystore -storepass android -keypass android ${MAIN_AAB} androiddebugkey


if [ ${LOCAL_TESTING} = true ]; then
	# create .apks file for testing
	echo "---- CREATE APKS FOR TESTING"
	java -cp ${BOB} com.dynamo.bob.tools.AndroidTools bundletool build-apks --bundle=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.aab --output=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.apks --local-testing
else
	# create universal .apks file
	echo "---- CREATE UNIVERSAL APKS"
	java -cp ${BOB} com.dynamo.bob.tools.AndroidTools bundletool build-apks --mode=UNIVERSAL --bundle=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.aab --output=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.apks --ks=debug.keystore --ks-pass=pass:android --key-pass=pass:android --ks-key-alias=androiddebugkey
fi


if [ ${VARIANT} = "debug" ]; then
	# install on device
	echo "---- INSTALL ON DEVICE"
	java -cp ${BOB} com.dynamo.bob.tools.AndroidTools bundletool install-apks --apks=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.apks
fi