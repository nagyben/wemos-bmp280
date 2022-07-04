locals {
  timestamp = formatdate("YYMMDDhhmmss", timestamp())
  root_dir  = abspath("../../../../../../cloud_functions")

  cf_invokers = [
    "serviceAccount:${google_service_account.esp8266_sa.email}",
    "serviceAccount:${data.google_service_account.ci_sa.email}"
  ]
}

data "archive_file" "receiver_source" {
  type        = "zip"
  source_dir  = "${local.root_dir}/receiver"
  output_path = "/tmp/receiver.zip"
  excludes = concat(
    tolist(fileset(local.root_dir, "receiver/e2e/**")),
    tolist(fileset(local.root_dir, "receiver/tests/**")),
    tolist(fileset(local.root_dir, "receiver/.pytest_cache/**")),
    tolist(fileset(local.root_dir, "receiver/Makefile**"))
  )
}

resource "google_storage_bucket_object" "receiver_zip" {
  # Append file MD5 to force object to be recreated
  name     = "receiver${var.env_suffix}-${data.archive_file.receiver_source.output_md5}.zip"
  bucket   = google_storage_bucket.functions_storage_bucket.name
  source   = data.archive_file.receiver_source.output_path
  # metadata = {}
}

resource "google_cloudfunctions_function" "receiver_function_authenticated" {
  name    = "receiver_authenticated${var.env_suffix}"
  runtime = "python39"

  available_memory_mb   = 128
  source_archive_bucket = google_storage_bucket.functions_storage_bucket.name
  source_archive_object = google_storage_bucket_object.receiver_zip.name

  event_trigger {
    event_type = "google.pubsub.topic.publish"
    resource = google_pubsub_topic.pubsub_weather_topic.id
  }

  environment_variables = {
    FIRESTORE_COLLECTION_NAME = "weather${var.env_suffix}"
  }

  entry_point           = "receiver_function"
  max_instances         = 1
}

resource "google_cloudfunctions_function_iam_member" "receiver_function_authenticated_invoker" {
  count          = length(local.cf_invokers)
  project        = google_cloudfunctions_function.receiver_function_authenticated.project
  region         = google_cloudfunctions_function.receiver_function_authenticated.region
  cloud_function = google_cloudfunctions_function.receiver_function_authenticated.name

  role   = "roles/cloudfunctions.invoker"
  member = local.cf_invokers[count.index]
}

data "archive_file" "viz_source" {
  type        = "zip"
  source_dir  = "${local.root_dir}/viz"
  output_path = "/tmp/viz.zip"
  excludes = concat(
    tolist(fileset(local.root_dir, "viz/e2e/**")),
    tolist(fileset(local.root_dir, "viz/tests/**")),
    tolist(fileset(local.root_dir, "viz/.pytest_cache/**")),
    tolist(fileset(local.root_dir, "viz/Makefile**"))
  )
}

resource "google_storage_bucket_object" "viz_zip" {
  # Append file MD5 to force object to be recreated
  name     = "viz${var.env_suffix}-${data.archive_file.viz_source.output_md5}.zip"
  bucket   = google_storage_bucket.functions_storage_bucket.name
  source   = data.archive_file.viz_source.output_path
  # metadata = {}
}

resource "google_cloudfunctions_function" "viz_function_authenticated" {
  name    = "viz_authenticated${var.env_suffix}"
  runtime = "python39"

  available_memory_mb   = 256
  source_archive_bucket = google_storage_bucket.functions_storage_bucket.name
  source_archive_object = google_storage_bucket_object.viz_zip.name

  event_trigger {
    event_type = "google.pubsub.topic.publish"
    resource = google_pubsub_topic.pubsub_weather_topic.id
  }

  environment_variables = {
    FIREBASE_COLLECTION = "weather${var.env_suffix}"
  }

  entry_point           = "viz_function"
  max_instances         = 1
}
