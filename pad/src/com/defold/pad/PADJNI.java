package com.defold.pad;

import androidx.annotation.NonNull;

import android.content.Context;
import android.app.Activity;
import android.util.Log;

import java.util.Arrays;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.util.Collections;
import java.util.concurrent.ConcurrentHashMap;

import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;

import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnCanceledListener;
import com.google.android.gms.tasks.OnFailureListener;

import com.google.android.play.core.assetpacks.AssetPackManagerFactory;
import com.google.android.play.core.assetpacks.AssetPackManager;
import com.google.android.play.core.assetpacks.AssetPackStates;
import com.google.android.play.core.assetpacks.AssetPackState;
import com.google.android.play.core.assetpacks.AssetPackLocation;
import com.google.android.play.core.assetpacks.AssetPackStateUpdateListener;
import com.google.android.play.core.assetpacks.model.AssetPackErrorCode;
import com.google.android.play.core.assetpacks.model.AssetPackStatus;


public class PADJNI implements AssetPackStateUpdateListener {
    private static final String TAG = "defold";

    // repeated in pad.cpp
    private enum PADEventType {
        EVENT_PACK_STATE_UPDATED,
        EVENT_PACK_STATE_ERROR,
        
        EVENT_REMOVE_PACK_COMPLETED,
        EVENT_REMOVE_PACK_CANCELED,
        EVENT_REMOVE_PACK_ERROR,

        EVENT_DIALOG_CONFIRMED,
        EVENT_DIALOG_DECLINED,
        EVENT_DIALOG_CANCELED,
        EVENT_DIALOG_ERROR,
    }

    private static class PADEvent {
        public String packName;
        public PADEventType eventType;
        public String extra;

        public PADEvent(String packName, PADEventType eventType) {
            this.packName = packName;
            this.eventType = eventType;
        }
    }

    private Activity activity;
    private AssetPackManager assetPackManager;

    private Map<String, AssetPackState> stateCache = new ConcurrentHashMap<>();

    private List<PADEvent> events = Collections.synchronizedList(new ArrayList<>());

    public PADJNI(Activity activity) {
        this.activity = activity;
        Log.d(TAG, "Constructor");
        assetPackManager = AssetPackManagerFactory.getInstance(activity);
        assetPackManager.registerListener(this);
    }

    private void AddEvent(String packName, PADEventType eventType) {
        PADEvent event = new PADEvent(packName, eventType);
        synchronized (events) {
            events.add(event);
        }
    }
    private void AddEvent(String packName, PADEventType eventType, String extra) {
        PADEvent event = new PADEvent(packName, eventType);
        event.extra = extra;
        synchronized (events) {
            events.add(event);
        }
    }

    public String GetNextEvent() {
        // Log.d(TAG, "GetNextEvent");
        PADEvent event = null;
        synchronized (events) {
            if (events.isEmpty()) {
                // Log.d(TAG, "GetNextEvent no event");
                return null;
            }
            event = events.remove(0);
        }

        Log.d(TAG, "GetNextEvent creating event json");
        JSONObject jo = new JSONObject();
        try {
            jo.put("pack_name", event.packName);
            jo.put("event_type", event.eventType.ordinal());
            jo.put("extra", event.extra);
        }
        catch (JSONException e) {
            Log.e(TAG, "JSON object creation error: " + e.getMessage());
        }
        return jo.toString();
    }

    public void Cancel(String packName) {
        Log.d(TAG, "Cancel");
        List<String> packNames = Arrays.asList(packName);
        AssetPackStates states = assetPackManager.cancel(packNames);
        AssetPackState state = states.packStates().get(packName);
        stateCache.put(packName, state);
        AddEvent(packName, PADEventType.EVENT_PACK_STATE_UPDATED);
    }

    public void Fetch(String packName) {
        Log.d(TAG, "Fetch " + packName);
        List<String> packNames = Arrays.asList(packName);
        Task<AssetPackStates> task = assetPackManager.fetch(packNames);
        task.addOnCompleteListener(new OnCompleteListener<AssetPackStates>() {
            @Override
            public void onComplete(@NonNull Task<AssetPackStates> task) {
                Log.d(TAG, "Fetch onComplete");
                AssetPackStates states = task.getResult();
                AssetPackState state = states.packStates().get(packName);
                stateCache.put(packName, state);
                AddEvent(packName, PADEventType.EVENT_PACK_STATE_UPDATED);
            }
        });
        task.addOnCanceledListener(new OnCanceledListener() {
            @Override
            public void onCanceled() {
                Log.d(TAG, "Fetch onCanceled");
                AddEvent(packName, PADEventType.EVENT_PACK_STATE_ERROR);
            }
        });
        task.addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(Exception e) {
                Log.d(TAG, "Fetch onFailure");
                AddEvent(packName, PADEventType.EVENT_PACK_STATE_ERROR, e.getMessage());
            }
        });
    }

    public boolean HasPackState(String packName) {
        Log.d(TAG, "HasPackState " + packName);
        return stateCache.containsKey(packName);
    }


    public void GetPackState(String packName) {
        Log.d(TAG, "GetPackState " + packName);
        List<String> packNames = Arrays.asList(packName);
        Task<AssetPackStates> task = assetPackManager.getPackStates(packNames);
        task.addOnCompleteListener(new OnCompleteListener<AssetPackStates>() {
            @Override
            public void onComplete(@NonNull Task<AssetPackStates> task) {
                Log.d(TAG, "GetPackState onComplete");
                AssetPackStates states = task.getResult();
                AssetPackState state = states.packStates().get(packName);
                stateCache.put(packName, state);
                AddEvent(packName, PADEventType.EVENT_PACK_STATE_UPDATED);
            }
        });
        task.addOnCanceledListener(new OnCanceledListener() {
            @Override
            public void onCanceled() {
                Log.d(TAG, "GetPackState onCanceled");
                AddEvent(packName, PADEventType.EVENT_PACK_STATE_ERROR);
            }
        });
        task.addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(Exception e) {
                Log.d(TAG, "GetPackState onFailure");
                AddEvent(packName, PADEventType.EVENT_PACK_STATE_ERROR, e.getMessage());
            }
        });
    }

    public long GetPackBytesDownloaded(String packName) {
        Log.d(TAG, "GetPackBytesDownloaded " + packName);
        AssetPackState state = stateCache.get(packName);
        return (state != null) ? state.bytesDownloaded() : 0;
    }
    public int GetPackErrorCode(String packName) {
        Log.d(TAG, "GetPackErrorCode " + packName);
        AssetPackState state = stateCache.get(packName);
        return (state != null) ? state.errorCode() : AssetPackErrorCode.PACK_UNAVAILABLE;
    }
    public int GetPackStatus(String packName) {
        Log.d(TAG, "GetPackStatus " + packName);
        AssetPackState state = stateCache.get(packName);
        return (state != null) ? state.status() : AssetPackStatus.UNKNOWN;
    }
    public long GetPackTotalBytesToDownload(String packName) {
        Log.d(TAG, "GetPackTotalBytesToDownload " + packName);
        AssetPackState state = stateCache.get(packName);
        return (state != null) ? state.totalBytesToDownload() : 0;
    }
    public int GetPackTransferProgressPercentage(String packName) {
        Log.d(TAG, "GetPackTransferProgressPercentage " + packName);
        AssetPackState state = stateCache.get(packName);
        return (state != null) ? state.transferProgressPercentage() : 0;
    }
    public String GetPackLocation(String packName) {
        Log.d(TAG, "GetPackLocation " + packName);
        AssetPackLocation location = assetPackManager.getPackLocation(packName);
        String path = location.assetsPath();
        Log.d(TAG, "GetPackLocation path = " + path);
        return path;
    }

    public void RemovePack(String packName) {
        Log.d(TAG, "RemovePack " + packName);
        Task<Void> task = assetPackManager.removePack(packName);
        task.addOnCompleteListener(new OnCompleteListener<Void>() {
            @Override
            public void onComplete(@NonNull Task<Void> task) {
                Log.d(TAG, "RemovePack onComplete");
                AddEvent(packName, PADEventType.EVENT_REMOVE_PACK_COMPLETED);
            }
        });
        task.addOnCanceledListener(new OnCanceledListener() {
            @Override
            public void onCanceled() {
                Log.d(TAG, "RemovePack onCanceled");
                AddEvent(packName, PADEventType.EVENT_REMOVE_PACK_CANCELED);
            }
        });
        task.addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(Exception e) {
                Log.d(TAG, "RemovePack onFailure");
                AddEvent(packName, PADEventType.EVENT_REMOVE_PACK_ERROR, e.getMessage());
            }
        });
    }

    public void ShowConfirmationDialog(String packName) {
        Log.d(TAG, "ShowConfirmationDialog " + packName);
        // Task<Integer> task = assetPackManager.showConfirmationDialog(activity);
        // task.addOnCompleteListener(new OnCompleteListener<Integer>() {
        //     @Override
        //     public void onComplete(@NonNull Task<Integer> task) {
        //         PADEventType eventType = (task.getResult() == Activity.RESULT_OK) ? PADEventType.EVENT_DIALOG_CONFIRMED : PADEventType.EVENT_DIALOG_DECLINED;
        //         AddEvent(packName, eventType);
        //     }
        // });
        // task.addOnCanceledListener(new OnCanceledListener() {
        //     @Override
        //     public void onCanceled() {
        //         AddEvent(packName, PADEventType.EVENT_DIALOG_CANCELED);
        //     }
        // });
        // task.addOnFailureListener(new OnFailureListener() {
        //     @Override
        //     public void onFailure(Exception e) {
        //         AddEvent(packName, PADEventType.EVENT_DIALOG_ERROR, e.getMessage());
        //     }
        // });

    }

    @Override
    public void onStateUpdate(AssetPackState state) {
        String packName = state.name();
        Log.d(TAG, "onStateUpdate " + packName);
        stateCache.put(packName, state);
        AddEvent(packName, PADEventType.EVENT_PACK_STATE_UPDATED);
    }
}
