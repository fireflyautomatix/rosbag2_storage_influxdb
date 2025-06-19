# rosbag2_storage_influxdb
This is a proof-of-concept rosbag2 write-only storage plugin for InfluxDB. It has some utility, since it gives you a fairly painless way to stream data into an actual queryable database. However, it is not suited for all the kinds of multi-modal data in ROS, and there are some significant performance and deployment limitations.

## Usage

```
INFLUXDB_HOST=localhost INFLUXDB_PORT=8086 INFLUXDB_TOKEN=<your_token_here> INFLUXDB_ORG=<your_org> INFLUXDB_BUCKET=<your_bucket> ros2 bag record -s influxdb -f influxdb
```

## Design
This works by using a converter interface to serialize CDR messages to to InfluxDB line protocol, which is then sent to the InfluxDB server in the storage plugin.

## Limitations
- The plugin is write-only, so you can't do `ros2 bag play`
- Array types are supported, but may not be efficient or useful, since each array index is stored as its own measurement (i.e. `data.0`, `data.1`, etc.). This means you probably shouldn't try to record Pointclouds for example.
- InfluxDB isn't really designed to run on edge devices, and has issues with high memory and CPU usage. Putting resource limits on it helps system stability, but results in more lost data. This project was only tested with running InfluxDB on the same device as the rosbag2 recording process, and not streaming data to a remote InfluxDB instance.
