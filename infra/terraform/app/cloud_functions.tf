locals {
  timestamp = formatdate("YYMMDDhhmmss", timestamp())
  root_dir  = abspath("../../../../../../cloud_functions")

  cf_invokers = [
    "serviceAccount:${google_service_account.esp8266_sa.email}",
    "serviceAccount:${data.google_service_account.ci_sa.email}"
  ]
}

data "archive_file" "data_ingestion_source" {
  type        = "zip"
  source_dir  = local.root_dir
  output_path = "/tmp/function.zip"
  excludes = concat(
    tolist(fileset(path.module, "../../../../../../cloud_functions/e2e/**")),
    tolist(fileset(path.module, "../../../../../../cloud_functions/tests/**")),
    tolist(fileset(path.module, "../../../../../../cloud_functions/.pytest_cache/**"))
  )
}

resource "google_storage_bucket_object" "zip" {
  # Append file MD5 to force object to be recreated
  name     = "function${var.env_suffix}-${data.archive_file.data_ingestion_source.output_md5}.zip"
  bucket   = google_storage_bucket.functions_storage_bucket.name
  source   = data.archive_file.data_ingestion_source.output_path
  # metadata = {}
}

resource "google_cloudfunctions_function" "receiver_function_authenticated" {
  name    = "receiver_authenticated${var.env_suffix}"
  runtime = "python39"

  available_memory_mb   = 128
  source_archive_bucket = google_storage_bucket.functions_storage_bucket.name
  source_archive_object = google_storage_bucket_object.zip.name
  trigger_http          = true
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