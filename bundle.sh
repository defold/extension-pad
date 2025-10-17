set -e

APP_NAME=PlayAssetDelivery
ASSETPACKS=$(realpath assetpacks)
BOB=$(realpath bob.jar)
VARIANT=release
LOCAL_TESTING=false


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
rm -rf ${ASSETPACKS}/out
mkdir ${ASSETPACKS}/out
for PACK_NAME in asset_pack_1 asset_pack_2
do
	IN_DIR=${ASSETPACKS}/${PACK_NAME}
	OUT_DIR=${ASSETPACKS}/out/${PACK_NAME}

	echo "---- LINK ASSET PACK RESOURCES"
	mkdir ${OUT_DIR}
	java -jar ${BOB} aapt2 -- link --proto-format --output-to-dir -o ${OUT_DIR} --manifest ${IN_DIR}/manifest/AndroidManifest.xml -A ${IN_DIR}/assets
	rm ${OUT_DIR}/resources.pb
	mkdir ${OUT_DIR}/manifest
	mv ${OUT_DIR}/AndroidManifest.xml ${OUT_DIR}/manifest/AndroidManifest.xml

	# create aab file for asset pack
	echo "---- CREATE AAB FILE FOR ASSET PACK"
	pushd ${OUT_DIR}
	zip -r -0 ${PACK_NAME}.zip .
	java -jar ${BOB} bundletool -- build-bundle --modules ${PACK_NAME}.zip --config ${IN_DIR}/bundleconfig.json --output ${ASSETPACKS}/out/${PACK_NAME}.aab
	rm ${PACK_NAME}.zip
	popd

	# remove intermediate files and unpack created aab file
	# add unzipped aab file to main aab
	echo "---- ADD ASSET PACK TO MAIN AAB"
	rm -rf ${OUT_DIR}
	mkdir ${OUT_DIR}
	unzip ${ASSETPACKS}/out/${PACK_NAME}.aab -d ${OUT_DIR}
	rm ${OUT_DIR}/bundleconfig.pb
	pushd ${OUT_DIR}
	# -D do not write directory entries to the archive
	zip -r -0 -D ${MAIN_AAB} .
	popd
done

# sign .aab
echo "---- SIGN AAB"
java -jar ${BOB} jarsigner -- -verbose -keystore debug.keystore -storepass android -keypass android ${MAIN_AAB} androiddebugkey


if [ ${LOCAL_TESTING} = true ]; then
	# create .apks file for testing
	echo "---- CREATE APKS FOR TESTING"
	java -jar ${BOB} bundletool -- build-apks --bundle=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.aab --output=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.apks --local-testing
else
	# create universal .apks file
	echo "---- CREATE UNIVERSAL APKS"
	java -jar ${BOB} bundletool -- build-apks --mode=UNIVERSAL --bundle=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.aab --output=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.apks --ks=debug.keystore --ks-pass=pass:android --key-pass=pass:android --ks-key-alias=androiddebugkey
fi


if [ ${VARIANT} = "debug" ]; then
	# install on device
	echo "---- INSTALL ON DEVICE"
	java -jar bob.jar bundletool -- install-apks --apks=${BUNDLE_DIR}/${APP_NAME}/${APP_NAME}.apks
fi