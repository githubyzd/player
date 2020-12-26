package com.sinochem.player;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.Manifest;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import com.chad.library.adapter.base.BaseQuickAdapter;
import com.chad.library.adapter.base.BaseViewHolder;
import com.sinochem.player.data.Constant;
import com.sinochem.player.data.TvItemBean;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public class TvListActivity extends AppCompatActivity implements BaseQuickAdapter.OnItemClickListener {

    private String TAG = "MainActivity";
    private RecyclerView recyclerView;
    private List<TvItemBean> data = new ArrayList<>();
    private MyAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_tv_list);

        recyclerView = findViewById(R.id.recyclerView);
        initView();
        initData();
        myRequetPermission();
    }

    private void initView() {
        setTitle("首页");
        recyclerView.setLayoutManager(new LinearLayoutManager(this, RecyclerView.VERTICAL, false));
        adapter = new MyAdapter(R.layout.item_tv_layout, data);
        recyclerView.setAdapter(adapter);
        adapter.setOnItemClickListener(this);
    }

    private void initData() {
        try {
            JSONArray array = new JSONArray(Constant.json);
            for (int i = 0; i < array.length(); i++) {
                TvItemBean item = new TvItemBean();
                JSONObject object = (JSONObject) array.get(i);
                item.setName(object.optString("title"));
                item.setUrl(object.optString("url"));
                data.add(item);
            }
            adapter.notifyDataSetChanged();
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }


    @Override
    public void onItemClick(BaseQuickAdapter adapter, View view, int position) {
        TvItemBean tvItemBean = data.get(position);
        Intent intent = new Intent(this, PlayActivity.class);
        intent.putExtra(PlayActivity.TITLE_KEY, tvItemBean.getName());
        intent.putExtra(PlayActivity.URL_KEY, tvItemBean.getUrl());
        startActivity(intent);
    }


    private class MyAdapter extends BaseQuickAdapter<TvItemBean, BaseViewHolder> {

        public MyAdapter(int layoutResId, @Nullable List<TvItemBean> data) {
            super(layoutResId, data);
        }

        @Override
        protected void convert(@NonNull BaseViewHolder helper, TvItemBean item) {
            helper.setText(R.id.name, item.getName());
        }
    }

    private void myRequetPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,Manifest.permission.READ_EXTERNAL_STORAGE}, 101);
        }else {
            Toast.makeText(this,"您已经申请了权限!",Toast.LENGTH_SHORT).show();
        }
    }
}