// Taken from https://github.com/prisma/prisma/blob/master/server/servers/api/src/main/scala/com/prisma/api/server/QueryExecutor.scala

package com.prisma.api.server

import com.prisma.api.ApiDependencies
import com.prisma.api.schema.{ApiUserContext, UserFacingError}
import com.prisma.messagebus.pubsub.{Everything, Message}
import com.prisma.sangria.utils.ErrorHandler
import com.prisma.shared.messages.SchemaInvalidatedMessage
import com.prisma.shared.models.Project
import play.api.libs.json.JsValue
import sangria.ast.Document
import sangria.execution.{Executor, QueryAnalysisError}
import sangria.schema.Schema
import sangria.validation.{QueryValidator, Violation}

import scala.collection.immutable.Seq
import scala.concurrent.Future

case class QueryExecutor()(implicit apiDependencies: ApiDependencies) {
  import apiDependencies.system.dispatcher
  import com.prisma.api.server.JsonMarshalling._

  val queryValidationCache = apiDependencies.cacheFactory.lfu[(String, Document), Vector[Violation]](sangriaMinimumCacheSize, sangriaMaximumCacheSize)

  apiDependencies.invalidationSubscriber.subscribe(Everything, (msg: Message[SchemaInvalidatedMessage]) => {
    queryValidationCache.removeAll(key => key._1 == msg.topic)
  })

  def execute(
      requestId: String,
      queryString: String,
      queryAst: Document,
      variables: JsValue,
      operationName: Option[String],
      project: Project,
      schema: Schema[ApiUserContext, Unit]
  ): Future[JsValue] = {
    val context = ApiUserContext(clientId = "clientId")
    val errorHandler = ErrorHandler(
      requestId,
      "POST",
      "",
      Seq.empty,
      queryString,
      variables,
      apiDependencies.reporter,
      projectId = Some(project.id),
      errorCodeExtractor = errorExtractor
    )

    val queryValidator = new QueryValidator {
      override def validateQuery(schema: Schema[_, _], queryAst: Document): Vector[Violation] = {
        queryValidationCache.getOrUpdate((project.id, queryAst), () => QueryValidator.default.validateQuery(schema, queryAst))
      }
    }

    val result: Future[JsValue] = Executor.execute(
      schema = schema,
      queryAst = queryAst,
      userContext = context,
      variables = variables,
      exceptionHandler = errorHandler.sangriaExceptionHandler,
      operationName = operationName,
      deferredResolver = apiDependencies.deferredResolverProvider(project),
      queryValidator = queryValidator
    )

    result.recover {
      case e: QueryAnalysisError => e.resolveError
    }
  }

  def errorExtractor(t: Throwable): Option[Int] = t match {
    case e: UserFacingError => Some(e.code)
    case _                  => None
  }


  def errorExtractor(t: Throwable): Option[Int] = t match {
    case e: UserFacingError => Some(e.code)
    case _                  => None
  }
}









// Taken from https://gist.github.com/ricardclau/93a45952bb423046b06b

package calculator

object TweetLength {
  final val MaxTweetLength = 140

  def tweetRemainingCharsCount(tweetText: Signal[String]): Signal[Int] = {
    Signal(MaxTweetLength - tweetLength(tweetText()))
  }

  def colorForRemainingCharsCount(remainingCharsCount: Signal[Int]): Signal[String] = {
    Signal[String] (
      remainingCharsCount() match {
        case x if x >= 15 => "green"
        case x if x >= 0 && x < 15 => "orange"
        case _ => "red"
      }
    )
  }

  /** Computes the length of a tweet, given its text string.
   *  This is not equivalent to text.length, as tweet lengths count the number
   *  of Unicode *code points* in the string.
   *  Note that this is still a simplified view of the reality. Full details
   *  can be found at
   *  https://dev.twitter.com/overview/api/counting-characters
   */
  private def tweetLength(text: String): Int = {
    /* This should be simply text.codePointCount(0, text.length), but it
     * is not implemented in Scala.js 0.6.2.
     */
    if (text.isEmpty) 0
    else {
      text.length - text.init.zip(text.tail).count(
          (Character.isSurrogatePair _).tupled)
    }
  }
}