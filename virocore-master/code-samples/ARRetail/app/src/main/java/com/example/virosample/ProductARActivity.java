/*
 * Copyright (c) 2017-present, Viro, Inc.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.example.virosample;
//hel
import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import android.support.v7.app.AppCompatActivity;

import com.example.virosample.model.Product;
import com.viro.core.ARAnchor;
import com.viro.core.ARHitTestListener;
import com.viro.core.ARHitTestResult;
import com.viro.core.ARNode;
import com.viro.core.ARScene;
import com.viro.core.AmbientLight;
import com.viro.core.AsyncObject3DListener;
import com.viro.core.ClickListener;
import com.viro.core.ClickState;
import com.viro.core.DragListener;
import com.viro.core.GestureRotateListener;
import com.viro.core.Material;
import com.viro.core.Node;
import com.viro.core.Object3D;
import com.viro.core.RendererConfiguration;
import com.viro.core.RotateState;
import com.viro.core.Spotlight;
import com.viro.core.Surface;
import com.viro.core.Vector;
import com.viro.core.ViroMediaRecorder;
import com.viro.core.ViroViewARCore;

import java.io.File;
import java.util.Arrays;

/**
 *  A ViroCore ProductARActivity that coordinates the placing of a Product last selected in the
 * {@link ProductSelectionActivity} in AR.
 */
public class ProductARActivity extends Activity {
    private static final int RECORD_PERM_KEY = 50;
    private static final String TAG = ProductARActivity.class.getSimpleName();
    final public static String INTENT_PRODUCT_KEY = "product_key";

    private ViroViewARCore mViroView;
    private ARScene mScene;
    private View mHudGroupView;
    private TextView mHUDInstructions;
    private ImageView mCameraButton;
    private View mIconShakeView;
    /*View.OnClickListener bookSearchEvent = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            displayScene();
        }
    };*/



    /*
     The Tracking status is used to coordinate the displaying of our 3D controls and HUD
     UI as the user looks around the tracked AR Scene.
     */
    private enum TRACK_STATUS{
        FINDING_SURFACE,
        SURFACE_NOT_FOUND,
        SURFACE_FOUND,
        SELECTED_SURFACE;
    }

    private TRACK_STATUS mStatus = TRACK_STATUS.SURFACE_NOT_FOUND;
    private Product mSelectedProduct = null;
    private Node mProductModelGroup = null;
    private Node mCrosshairModel = null;
    private AmbientLight mMainLight = null;
    private Vector mLastProductRotation = new Vector();
    private Vector mSavedRotateToRotation = new Vector();
    private ARHitTestListenerCrossHair mCrossHairHitTest = null;

    /*
     * ARNode under which to parent our 3D furniture model. This is only created
     * and non-ull if a user has selected a surface upon which to place the furniture.
     */
    private ARNode mHitARNode = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.ar_hud);
        //findViewById(R.id.button1).setOnClickListener(bookSearchEvent);
        RendererConfiguration config = new RendererConfiguration();
        config.setShadowsEnabled(true);
        config.setBloomEnabled(true);
        config.setHDREnabled(true);
        config.setPBREnabled(true);


        mViroView = new ViroViewARCore(this, new ViroViewARCore.StartupListener() {
            @Override
            public void onSuccess() {
                displayScene();
            }

            @Override
            public void onFailure(ViroViewARCore.StartupError error, String errorMessage) {
                Log.e(TAG, "Failed to load AR Scene [" + errorMessage + "]");
            }
        }, config);
        setContentView(mViroView);

        Intent intent = getIntent();
        String key = intent.getStringExtra(INTENT_PRODUCT_KEY);
        ProductApplicationContext context = (ProductApplicationContext)getApplicationContext();
        mSelectedProduct = context.getProductDB().getProductByName(key);

        View.inflate(this, R.layout.ar_hud, ((ViewGroup) mViroView));
        mHudGroupView = (View) findViewById(R.id.main_hud_layout);
        mHudGroupView.setVisibility(View.GONE);
    }

    @Override
    protected void onStart() {
        super.onStart();
        mViroView.onActivityStarted(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mViroView.onActivityResumed(this);
    }

    @Override
    protected void onPause(){
        super.onPause();
        mViroView.onActivityPaused(this);
    }

    @Override
    protected void onStop() {
        super.onStop();
        mViroView.onActivityStopped(this);
    }

    @Override
    protected void onDestroy(){
        ((ViroViewARCore)mViroView).setCameraARHitTestListener(null);
        mViroView.onActivityDestroyed(this);
        super.onDestroy();
    }

    private void requestPermissions(){
        ActivityCompat.requestPermissions(this,
                new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                RECORD_PERM_KEY);
    }

    private static boolean hasRecordingStoragePermissions(Context context) {
        boolean hasExternalStoragePerm = ContextCompat.checkSelfPermission(context,
                Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        return hasExternalStoragePerm;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        if (requestCode == RECORD_PERM_KEY && !hasRecordingStoragePermissions(ProductARActivity.this)) {
            Toast toast = Toast.makeText(ProductARActivity.this, "User denied permissions", Toast.LENGTH_LONG);
            toast.show();
        }
    }

    private void displayScene() {
        // Create the ARScene within which to load our ProductAR Experience
        mScene = new ARScene();
        //ARScene arScene = new ARScene();
        mMainLight = new AmbientLight(Color.parseColor("#606060"), 400);
        mMainLight.setInfluenceBitMask(3);
        mScene.getRootNode().addLight(mMainLight);
        //arScene.getRootNode().addLight(mMainLight);

        // Setup our 3D and HUD controls
        initARCrosshair(mScene);
        init3DModelProduct(mScene);
        //initARHud();

        // Start our tracking UI when the scene is ready to be tracked
        mScene.setListener(new ARSceneListener());

        // Finally set the arScene on the renderer
        mViroView.setScene(mScene);
    }

    private void initARHud(){
        // TextView instructions
        mHUDInstructions = (TextView) mViroView.findViewById(R.id.ar_hud_instructions);
        mViroView.findViewById(R.id.bottom_frame_controls).setVisibility(View.VISIBLE);

        // Bind the back button on the top left of the layout
        ImageView view = (ImageView) findViewById(R.id.ar_back_button);
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                ProductARActivity.this.finish();
            }
        });

        // Bind the detail buttons on the top right of the layout.
        ImageView productDetails = (ImageView) findViewById(R.id.ar_details_page);
        productDetails.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ProductARActivity.this, ProductDetailActivity.class);
                intent.putExtra(INTENT_PRODUCT_KEY, mSelectedProduct.mName);
                startActivity(intent);
            }
        });

        // Bind the camera button on the bottom, for taking images.
        mCameraButton  = (ImageView) mViroView.findViewById(R.id.ar_photo_button);
        final File photoFile = new File(getFilesDir(), "screenShot");
        mCameraButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!hasRecordingStoragePermissions(getBaseContext())){
                    requestPermissions();
                    return;
                }

                mViroView.getRecorder().takeScreenShotAsync("screenShot", true, new ViroMediaRecorder.ScreenshotFinishListener() {
                    @Override
                    public void onSuccess(Bitmap bitmap, String s) {
                        final Intent shareIntent = new Intent(Intent.ACTION_SEND);
                        shareIntent.setType("image/png");
                        shareIntent.putExtra(Intent.EXTRA_STREAM, Uri.parse(s));
                        startActivity(Intent.createChooser(shareIntent, "Share image using"));
                    }

                    @Override
                    public void onError(ViroMediaRecorder.Error error) {
                        Log.e("Viro","onTaskFailed " + error.toString());
                    }
                });
            }
        });

        mIconShakeView = mViroView.findViewById(R.id.icon_shake_phone);
    }
    /*public class MainActivity extends AppCompatActivity implements
            View.OnClickListener, com.example.virosample.MainActivity {
        @Override
        public void onClick(View view){
            Toast.makeText(this,"fgsafggsdgsfgdsgs",Toast.LENGTH_SHORT).show();
        }
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setContentView(R.layout.ar_hud);
            findViewById(R.id.button1).setOnClickListener(this);
        }
    }*/
    //マーカーが出る
    private void initARCrosshair(ARScene scene){
        if (mCrosshairModel != null){
            return;
        }

        AmbientLight am = new AmbientLight();
        am.setInfluenceBitMask(2);
        am.setIntensity(1000);
        mScene.getRootNode().addLight(am);

        final Object3D crosshairModel = new Object3D();
        mScene.getRootNode().addChildNode(crosshairModel);
        crosshairModel.loadModel(mViroView.getViroContext(), Uri.parse("file:///android_asset/tracking_1.vrx"), Object3D.Type.FBX, new AsyncObject3DListener() {
            @Override

            public void onObject3DLoaded(Object3D object3D, Object3D.Type type) {

                mCrosshairModel = object3D;
                mCrosshairModel.setOpacity(0);
                object3D.setLightReceivingBitMask(2);
                mCrosshairModel.setScale(new Vector(0.175,0.175,0.175));
                mCrosshairModel.setClickListener(new ClickListener(){
                    @Override
                    public void onClick(int i, Node node, Vector vector) {
                        setTrackingStatus(TRACK_STATUS.SELECTED_SURFACE);
                    }

                    @Override
                    public void onClickState(int i, Node node, ClickState clickState, Vector vector) {
                        // No-op
                    }
                });
            }

            @Override
            public void onObject3DFailed(String error) {
                Log.e("Viro"," Model load failed : " + error);
            }
        });
    }

    //public class MainActivity extends Activity {
        //@Override
        /*protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setContentView(R.layout.ar_hud);
            final Button button = (Button) findViewById(R.id.button1);
            button.setOnClickListener(new View.OnClickListener() {*/
                //@Override
                /*public void onClick(View v) {*/
                // Create our group node containing the light, shadow plane, and 3D models

    //カメラ部分をタップしたら家具出る
                private void init3DModelProduct(ARScene scene) {
                    mProductModelGroup = new Node();

                    // Create a light to be shined on the model.
                    Spotlight spotLight = new Spotlight();
                    spotLight.setInfluenceBitMask(1);
                    spotLight.setPosition(new Vector(0, -1, 0));
                    spotLight.setCastsShadow(true);
                    spotLight.setAttenuationEndDistance(9);
                    spotLight.setAttenuationStartDistance(4);
                    spotLight.setDirection(new Vector(0, -1, 0));
                    spotLight.setIntensity(6000);
                    spotLight.setShadowOpacity(0.35f);
                    mProductModelGroup.addLight(spotLight);

                    // Create a mock shadow plane in AR
                    Node shadowNode = new Node();
                    Surface shadowSurface = new Surface(20, 20);
                    Material material = new Material();
                    material.setShadowMode(Material.ShadowMode.TRANSPARENT);
                    material.setLightingModel(Material.LightingModel.LAMBERT);
                    shadowSurface.setMaterials(Arrays.asList(material));
                    shadowNode.setGeometry(shadowSurface);
                    shadowNode.setLightReceivingBitMask(1);
                    shadowNode.setPosition(new Vector(0, -1, 0));
                    shadowNode.setRotation(new Vector(-1.5708, 0, 0));
                    // We want the shadow node to ignore all events because it contains a surface of size 20x20
                    // meters and causes this to capture events which will bubble up to the mProductModelGroup node.
                    shadowNode.setIgnoreEventHandling(true);
                    mProductModelGroup.addChildNode(shadowNode);

                    // Load the model from the given mSelected Product
                    final Object3D productModel = new Object3D();

                    productModel.loadModel(mViroView.getViroContext(), Uri.parse("file:///android_asset/Chair.vrx"/*mSelectedProduct.m3DModelUri*/), Object3D.Type.FBX, new AsyncObject3DListener() {
                        @Override
                        public void onObject3DLoaded(Object3D object3D, Object3D.Type type) {
                            object3D.setLightReceivingBitMask(1);
                            mProductModelGroup.setOpacity(1);
                            mProductModelGroup.setScale(new Vector(0.9, 0.9, 0.9));
                            mLastProductRotation = object3D.getRotationEulerRealtime();
                        }

                        @Override
                        public void onObject3DFailed(String error) {
                            Log.e("Viro", " Model load failed : " + error);
                        }
                    });

                    // Make this 3D Product object draggable.
                    mProductModelGroup.setDragType(Node.DragType.FIXED_TO_WORLD);
                    mProductModelGroup.setDragListener(new DragListener() {
                        @Override
                        public void onDrag(int i, Node node, Vector vector, Vector vector1) {
                            // No-op
                        }
                    });

                    // Set gesture listeners such that the user can rotate this model.
                    productModel.setGestureRotateListener(new GestureRotateListener() {
                        @Override
                        public void onRotate(int source, Node node, float radians, RotateState rotateState) {
                            if (rotateState == RotateState.ROTATE_END) {
                                mLastProductRotation = mSavedRotateToRotation;
                            } else {
                                Vector rotateTo = new Vector(mLastProductRotation.x, mLastProductRotation.y + radians, mLastProductRotation.z);
                                mProductModelGroup.setRotation(rotateTo);
                                mSavedRotateToRotation = rotateTo;
                            }
                        }
                    });

                    mProductModelGroup.setOpacity(1);
                    mProductModelGroup.addChildNode(productModel);
                }
            //}
            //});
       //}
   // }


    private void setTrackingStatus(TRACK_STATUS status) {
        if (mStatus == TRACK_STATUS.SELECTED_SURFACE || mStatus == status){
            return;
        }

        // If the surface has been selected, we no longer need our cross hair listener.
        if (status == TRACK_STATUS.SELECTED_SURFACE){
            ((ViroViewARCore)mViroView).setCameraARHitTestListener(null);
        }

        mStatus = status;
        updateUIHud();
        update3DARCrosshair();
        update3DModelProduct();
    }

    private void updateUIHud(){
        /*switch(mStatus){
            case FINDING_SURFACE:
                mHUDInstructions.setText("Point the camera at the flat surface where you want to view your product.");
                break;
            case SURFACE_NOT_FOUND:
                mHUDInstructions.setText("We can’t seem to find a surface. Try moving your phone more in any direction.");
                break;
            case SURFACE_FOUND:
                mHUDInstructions.setText("Great! Now tap where you want to see the product.");
                break;
            case SELECTED_SURFACE:
                mHUDInstructions.setText("Great! Use one finger to move and two fingers to rotate.");
                break;
            default:
                mHUDInstructions.setText("Initializing AR....");
        }

        // Update the camera UI
        if (mStatus == TRACK_STATUS.SELECTED_SURFACE){
            mCameraButton.setVisibility(View.VISIBLE);
        } else {
            mCameraButton.setVisibility(View.GONE);
        }

        // Update the Icon shake view
        if (mStatus == TRACK_STATUS.SURFACE_NOT_FOUND){
            mIconShakeView.setVisibility(View.VISIBLE);
        } else {
            mIconShakeView.setVisibility(View.GONE);
        }*/
    }

    private void update3DARCrosshair(){
        switch(mStatus){
            case FINDING_SURFACE:
            case SURFACE_NOT_FOUND:
            case SELECTED_SURFACE:
                mCrosshairModel.setOpacity(0);
                break;
            case SURFACE_FOUND:
                mCrosshairModel.setOpacity(1);
                break;
        }

        if (mStatus == TRACK_STATUS.SELECTED_SURFACE && mCrossHairHitTest != null){
            mCrossHairHitTest = null;
            ((ViroViewARCore)mViroView).setCameraARHitTestListener(null);
        } else if (mCrossHairHitTest == null) {
            mCrossHairHitTest = new ARHitTestListenerCrossHair();
            ((ViroViewARCore)mViroView).setCameraARHitTestListener(mCrossHairHitTest);
        }
    }

    private void update3DModelProduct(){
        // Hide the product if the user has not placed it yet.
        if (mStatus != TRACK_STATUS.SELECTED_SURFACE){
            mProductModelGroup.setOpacity(1);
            return;
        }

        if (mHitARNode != null){
            return;
        }

        mHitARNode = mScene.createAnchoredNode(mCrosshairModel.getPositionRealtime());
        mHitARNode.addChildNode(mProductModelGroup);
        mProductModelGroup.setOpacity(1);
    }

    private class ARHitTestListenerCrossHair implements ARHitTestListener {
        @Override
        public void onHitTestFinished(ARHitTestResult[] arHitTestResults) {
            if( arHitTestResults == null || arHitTestResults.length <=0) {
                return;
            }

            // If we have found intersected AR Hit points, update views as needed, reset miss count.
            ViroViewARCore viewARView = (ViroViewARCore)mViroView;
            final Vector cameraPos  = viewARView.getLastCameraPositionRealtime();

            // Grab the closest ar hit target
            float closestDistance = Float.MAX_VALUE;
            ARHitTestResult result = null;
            for (int i = 0; i < arHitTestResults.length; i++) {
                ARHitTestResult currentResult = arHitTestResults[i];

                float distance = currentResult.getPosition().distance(cameraPos);
                if (distance < closestDistance && distance > .3 && distance < 5) {
                    result = currentResult;
                    closestDistance = distance;
                }
            }

            // Update the cross hair target location with the closest target.
            if (result != null) {
                mCrosshairModel.setPosition(result.getPosition());
                mCrosshairModel.setRotation(result.getRotation());
            }

            // Update State based on hit target
            if (result != null) {
                setTrackingStatus(TRACK_STATUS.SURFACE_FOUND);
            } else {
                setTrackingStatus(TRACK_STATUS.FINDING_SURFACE);
            }
        }
    }

    protected class ARSceneListener implements ARScene.Listener {
        private boolean mInitialized;

        public ARSceneListener() {
            mInitialized = false;
        }
        @Override
        public void onTrackingInitialized() {
          // This method is deprecated.
        }

        @Override
        public void onTrackingUpdated(ARScene.TrackingState trackingState, ARScene.TrackingStateReason trackingStateReason){
            if (trackingState == ARScene.TrackingState.NORMAL && !mInitialized) {
                // The Renderer is ready - turn everything visible.
                mHudGroupView.setVisibility(View.VISIBLE);

                // Update our UI views to the finding surface state.
                setTrackingStatus(TRACK_STATUS.FINDING_SURFACE);
                mInitialized = true;
            }
        }

        @Override
        public void onAmbientLightUpdate(float lightIntensity, Vector v) {
            // no-op
        }

        @Override
        public void onAnchorFound(ARAnchor anchor, ARNode arNode) {
            // no-op
        }

        @Override
        public void onAnchorUpdated(ARAnchor anchor, ARNode arNode) {
            // no-op
        }

        @Override
        public void onAnchorRemoved(ARAnchor anchor, ARNode arNode) {
            // no-op
        }
    }

}
